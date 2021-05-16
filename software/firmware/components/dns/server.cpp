#include "dns/dns.h"
#include "dns/server.h"
#include "dns/logging.h"
#include "error.h"
#include "events.h"
#include "settings.h"
#include "lists.h"

#include "errno.h"
#include "stdio.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"

#include <stdexcept>

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
#include "esp_log.h"
static const char *TAG = "DNS";

#define PACKET_QUEUE_SIZE 50
#define CLIENT_QUEUE_SIZE 50

static int dns_srv_sock;                                // Socket handle for sending queries to upstream DNS 
static TaskHandle_t dns;                                // Handle for DNS task
static TaskHandle_t listening;                          // Handle for listening task
static QueueHandle_t packet_queue;                      // FreeRTOS queue of DNS query packets
static SemaphoreHandle_t client_mutex;
static std::vector<Client> client_queue;                // FIFO Array of clients waiting for DNS response


static IRAM_ATTR void listening_t(void* parameters)
{
    ESP_LOGV(TAG, "Listening...");
    std::vector<uint8_t> buffer;
    while(1)
    {
        sockaddr_in addr;
        socklen_t addrlen;

        buffer.resize(MAX_PACKET_SIZE);
        size_t size = recvfrom(dns_srv_sock, buffer.data(), MAX_PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrlen);
        if( size < 1 )
        {
            ESP_LOGW(TAG, "Error Receiving Packet");
            continue;
        }
        buffer.resize(size);
        ESP_LOGV(TAG, "Received %d Byte Packet from %s", size, inet_ntoa(addr.sin_addr.s_addr));

        DNS* packet;
        try{
            packet = new DNS(&buffer, addr, addrlen);
        }catch(const std::out_of_range& oor){
            ESP_LOGV(TAG, "Received packet too small");
            continue;
        }
        
        if( xQueueSend(packet_queue, &packet, 0) == errQUEUE_FULL )
        {
            ESP_LOGE(TAG, "Queue Full, could not add packet");
            delete packet;
            continue;
        }
    }
}


static IRAM_ATTR esp_err_t forward_answer(DNS* packet)
{
    if( xSemaphoreTake(client_mutex, 25/portTICK_PERIOD_MS) == pdFALSE )
    {
        ESP_LOGW(TAG, "Timeout while adding client to queue");
        return ESP_FAIL;
    }

    for(int i = 0; i < client_queue.size(); i++)
    {
        if( packet->header.id == client_queue[i].id )
        {
            ESP_LOGV(TAG, "Forwarding answer to %s", inet_ntoa(client_queue[i].src_address.sin_addr.s_addr));
            packet->send(dns_srv_sock, client_queue[i].src_address);
            break;
        }
    }
    xSemaphoreGive(client_mutex);
    return ESP_OK;
}

static IRAM_ATTR esp_err_t add_client(DNS* packet)
{
    if( xSemaphoreTake(client_mutex, 25/portTICK_PERIOD_MS) == pdFALSE )
    {
        ESP_LOGW(TAG, "Timeout while adding client to queue");
        return ESP_FAIL;
    }

    Client client;
    client.src_address = packet->addr;
    client.id = packet->header.id;
    client.response_latency = packet->recv_timestamp;

    client_queue.push_back(client);
    if( client_queue.size() == CLIENT_QUEUE_SIZE )
        client_queue.erase(client_queue.begin());

    xSemaphoreGive(client_mutex);
    return ESP_OK;
}

static IRAM_ATTR void dns_t(void* parameters)
{
    struct sockaddr_in upstream_dns;
    upstream_dns.sin_family = PF_INET;
    upstream_dns.sin_port = htons(DNS_PORT);
    std::string ip = setting::read_str(setting::DNS_SRV);
    ip4addr_aton(ip.c_str(), (ip4_addr_t *)&upstream_dns.sin_addr.s_addr);
    ESP_LOGV(TAG, "Upstream DNS: %s", inet_ntoa(upstream_dns.sin_addr.s_addr));

    char device_url[MAX_URL_LENGTH];
    std::string url = setting::read_str(setting::HOSTNAME);
    strcpy(device_url, url.c_str());
    ESP_LOGV(TAG, "Device URL: %s", device_url);

    DNS* packet = NULL;
    while(1) 
    {
        delete packet;

        BaseType_t xErr = xQueueReceive(packet_queue, &packet, portMAX_DELAY);
        if(xErr == pdFALSE)
        {
            ESP_LOGW(TAG, "Error receiving from queue");
            continue;
        }

        std::string domain = packet->convert_qname_url();
        ESP_LOGD(TAG, "Domain  (%s)",   domain.c_str());

        if( packet->header.qr == ANSWER ) // Forward all answers
        {
            ESP_LOGV(TAG, "Forwarding answer for %s", domain.c_str());
            forward_answer(packet);
        }
        else if( packet->header.qr == QUERY )
        {
            uint16_t qtype = packet->question.qtype;
            if( !(qtype == A || qtype == AAAA) ) // Forward all queries that are not A & AAAA
            {
                ESP_LOGI(TAG, "Forwarding question for %s", domain.c_str());
                if( add_client(packet) == ESP_OK )
                    packet->send(dns_srv_sock, upstream_dns);
                log_query(domain, false, qtype, packet->addr.sin_addr.s_addr);
            }
            else 
            {
                vTaskDelay(0); // This yields to higher priority tasks, watchdog may get triggered without this
                if( memcmp(domain.c_str(), device_url, domain.size()) == 0 ) // Check is qname matches current device url
                {
                    ESP_LOGW(TAG, "Capturing DNS request %s", domain.c_str());
                    std::string ip_str = setting::read_str(setting::IP);
                    packet->records.clear();
                    packet->header.arcount = 0;
                    packet->question.qtype = A;
                    packet->add_answer(ip_str.c_str());
                    packet->send(dns_srv_sock, packet->addr);
                    log_query(domain, false, qtype, packet->addr.sin_addr.s_addr);
                    set_bit(BLOCKED_QUERY_BIT);
                }
                else if( setting::read_bool(setting::BLOCK) && in_blacklist(domain.c_str()) ) // check if url is in blacklist
                {
                    ESP_LOGW(TAG, "Blocking question for %s", domain.c_str());
                    packet->records.clear();
                    packet->header.arcount = 0;
                    if( qtype == A )
                        packet->add_answer("0.0.0.0");
                    else if( qtype == AAAA )
                        packet->add_answer("::");
                    packet->send(dns_srv_sock, packet->addr);
                    log_query(domain, true, qtype, packet->addr.sin_addr.s_addr);
                    set_bit(BLOCKED_QUERY_BIT);
                }
                else
                {
                    ESP_LOGI(TAG, "Forwarding question for %s", domain.c_str());
                    if( add_client(packet) == ESP_OK )
                        packet->send(dns_srv_sock, upstream_dns);
                    log_query(domain, false, qtype, packet->addr.sin_addr.s_addr);
                }
            }
        }

        int64_t end = esp_timer_get_time();
        ESP_LOGV(TAG, "Processing Time: %lld ms", (end-packet->recv_timestamp)/1000);
    }
}

void start_dns()
{
    ESP_LOGI(TAG, "Initializing DNS...");
    client_mutex = xSemaphoreCreateMutex();
    packet_queue = xQueueCreate(PACKET_QUEUE_SIZE, sizeof(DNS*));
    if( packet_queue == NULL || client_mutex == NULL)
    {
        THROWE(ESP_ERR_NO_MEM, "Error Initializing FreeRTOS structures for dns server")
    }

    // initialize listening socket
    ESP_LOGV(TAG, "Initializing Socket");
	if( (dns_srv_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
	{
        THROWE(errno, "Socket init failed %s", strerror(errno))
    }

    // Create socket for listening on port 53, allow from any IP address
    struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(DNS_PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if( bind(dns_srv_sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0 )
    {
        THROWE(errno, "Socket bind failed %s", strerror(errno))
    }

    BaseType_t xErr = xTaskCreatePinnedToCore(listening_t, "listening_task", 8000, NULL, 9, &listening, tskNO_AFFINITY);
    xErr &= xTaskCreatePinnedToCore(dns_t, "dns_task", 15000, NULL, 9, &dns, tskNO_AFFINITY);
    if( xErr != pdPASS )
    {
        THROWE(DNS_ERR_INIT, "Failed to start dns tasks");
    }

    bool blocking = setting::read_bool(setting::BLOCK);
    blocking ? set_bit(BLOCKING_BIT):clear_bit(BLOCKING_BIT);
    ESP_LOGV(TAG, "Blocking %s", blocking ? "on":"off");
}