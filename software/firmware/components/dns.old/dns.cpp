#include "dns.h"
#include "error.h"
#include "events.h"

#include "settings.h"
#include "url.h"
#include "logging.h"
#include "datetime.h"
// #include "gpio.h"

#include "errno.h"
#include "stdio.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "esp_netif.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "DNS";

#define PACKET_QUEUE_SIZE 10
#define CLIENT_QUEUE_SIZE 50

static socklen_t addrlen = sizeof(struct sockaddr_in);  // Convenience variable, used when sending packets
static int dns_srv_sock;                                // Socket handle for sending queries to upstream DNS 
static struct sockaddr_in upstream_dns;                 // sockaddr struct containing current upstream server IP
static TaskHandle_t dns;                                // Handle for DNS task
static TaskHandle_t listening;                          // Handle for listening task
static QueueHandle_t packet_queue;                      // FreeRTOS queue of DNS query packets
static Client client_queue[CLIENT_QUEUE_SIZE];          // FIFO Array of clients waiting for DNS response
static char device_url[MAX_URL_LENGTH];                 // Buffer to store current URL in RAM


static IRAM_ATTR void listening_t(void* parameters)
{
    ESP_LOGD(TAG, "Listening...");
    while(1)
    {
        Packet* packet = (Packet*)malloc(sizeof(*packet));
        packet->length = recvfrom(dns_srv_sock, packet->data, MAX_PACKET_SIZE, 0, (struct sockaddr *)&(packet->src), &addrlen);
        if( packet->length < 1 )
        {
            ESP_LOGW(TAG, "Error Receiving Packet");
            free(packet);
            continue;
        }

        ESP_LOGD(TAG, "Received %d Byte Packet from %s", packet->length, inet_ntoa(packet->src.sin_addr.s_addr));
        packet->recv_timestamp = esp_timer_get_time();

        if( xQueueSend(packet_queue, &packet, 0) == errQUEUE_FULL )
        {
            ESP_LOGE(TAG, "Queue Full, could not add packet");
            free(packet);
        }
    }
}

static IRAM_ATTR esp_err_t forward_query(Packet* packet)
{
    int sendlen = sendto(dns_srv_sock, packet->data, packet->length, 0, (struct sockaddr *)&upstream_dns, addrlen);

    // Wait ~25ms to resend if out of memory
    while( errno == ENOMEM && sendlen < 1)
    {
        vTaskDelay( 25 / portTICK_PERIOD_MS );
        sendlen = sendto(dns_srv_sock, packet->data, packet->length, 0, (struct sockaddr *)&upstream_dns, addrlen);
    }

    if(sendlen < 1)
    {
        ESP_LOGE(TAG, "Failed to forward query, errno=%s", strerror(errno));
        return ESP_FAIL;
    }
    else
    {
        // Move every entry in client queue to the next spot, removing last entry
        for(int i = CLIENT_QUEUE_SIZE-1; i >= 1; i--) 
        {
            client_queue[i] = client_queue[i-1];
        }

        // Add client to beginning of client queue
        client_queue[0].src_address = packet->src;
        client_queue[0].id = packet->dns.header->id;
        client_queue[0].response_latency = packet->recv_timestamp;
    }

    return ESP_OK;
}

static IRAM_ATTR esp_err_t answer_query(Packet* packet, uint32_t ip)
{
    packet->dns.header->qr = 1; // change packet to answer
    packet->dns.header->aa = 1; // respect my authoritah
    packet->dns.header->ans_count = htons(1);

    int len = 0;
    if( ntohs(packet->dns.query->qtype) == A )
    {
        // Create answer struct
        DNS_Answer answer = {
            answer.name = htons(0xC00C),
            answer.type = htons(1),
            answer.cls = htons(1),
            answer.ttl = htonl(128),
            answer.rdlength = htons(4),
            answer.rddata = htonl(ip)
        };
        
        // write answer to buffer after dns_query
        memcpy(packet->data + packet->length, &answer, sizeof(answer));
        len = sizeof(answer);
    }
    else if( ntohs(packet->dns.query->qtype) == AAAA )
    {
        // packet->dns.header->rd = 1; // recursion desired
        // packet->dns.header->ra = 1; // recursion available
        packet->dns.header->rcode = 3; // No such name
        
        /*
        TODO: implement ip6
        */
    }

    if( sendto(dns_srv_sock, packet->data, packet->length+len, 0, (struct sockaddr *)&packet->src, addrlen) < 1 )
    {
        ESP_LOGE(TAG, "Failed to answer query");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static IRAM_ATTR esp_err_t capture_query(Packet* packet)
{
    uint32_t ip = inet_addr("192.168.4.1"); // default ip in access point mode

    if( !check_bit(PROVISIONING_BIT) )
    {
        // If not provisioning, return current IP
        // esp_netif_ip_info_t info;
        // get_network_info(&info);
        // char ip_str[IP4_STRLEN_MAX];
        std::string ip_str = sett::read_str(sett::IP);
        ip = ntohl(inet_addr(ip_str.c_str()));
    }

    return answer_query(packet, ip);
}

static IRAM_ATTR esp_err_t block_query(Packet* packet)
{
    set_bit(BLOCKED_QUERY_BIT);
    // Answer query with 0.0.0.0
    return answer_query(packet, 0);
}

static IRAM_ATTR esp_err_t forward_answer(Packet* packet)
{
    // Go through client queue, searching for client with matching ID.
    // Do nothing if no client is found
    for(int i = 0; i < CLIENT_QUEUE_SIZE; i++)
    {
        if(packet->dns.header->id == client_queue[i].id)
        {
            int sendlen = sendto(dns_srv_sock, packet->data, packet->length, 0, (struct sockaddr *)&client_queue[i].src_address, addrlen);

            // Wait ~25ms to resend if out of memory
            while( errno == ENOMEM && sendlen < 1)
            {
                vTaskDelay( 25 / portTICK_PERIOD_MS );
                sendlen = sendto(dns_srv_sock, packet->data, packet->length, 0, (struct sockaddr *)&client_queue[i].src_address, addrlen);
            }

            if(sendlen < 1)
            {
                ESP_LOGE(TAG, "Failed to forward answer, errno=%s", strerror(errno));
                return ESP_FAIL;
            }

            break;
        }
    }
    return ESP_OK;
}

static IRAM_ATTR esp_err_t parse_packet(Packet* packet)
{
    // Point pointers at specific sections of packet (casting structs)

    packet->dns.header = (DNS_Header*)packet->data;
    packet->dns.qname = (char*)(packet->data + sizeof(*(packet->dns.header)));

    if( strlen(packet->dns.qname) > MAX_URL_LENGTH)
        return DNS_ERR_INVALID_QNAME;

    packet->dns.query = (DNS_Query*)(packet->dns.qname + strlen(packet->dns.qname) + 1);

    ESP_LOGD(TAG, "Packet:");
    ESP_LOGD(TAG, "ID      (%d)", packet->dns.header->id);
    ESP_LOGD(TAG, "QR      (%d)", packet->dns.header->qr);
    ESP_LOGD(TAG, "QNAME   (%s)(%d)", packet->dns.qname, strlen(packet->dns.qname));
    ESP_LOGD(TAG, "QTYPE   (%d)", ntohs(packet->dns.query->qtype));

    return ESP_OK;
}

static IRAM_ATTR void dns_t(void* parameters)
{
    Packet* packet = NULL;

    while(1) 
    {
        // Free previous packet memory, then receive new packet from queue
        if( packet )
            free(packet);

        BaseType_t xErr = xQueueReceive(packet_queue, &packet, portMAX_DELAY);
        if(xErr == pdFALSE)
        {
            ESP_LOGW(TAG, "Error receiving from queue");
            continue;
        }

        if( parse_packet(packet) != ESP_OK )
        {
            ESP_LOGI(TAG, "Error parsing packet");
            continue;
        }

        URL url = convert_qname_to_url(packet->dns.qname);
        if( packet->dns.header->qr == ANSWER ) // Forward all answers
        {
            ESP_LOGI(TAG, "Forwarding answer for %.*s", url.length, url.string);
            forward_answer(packet);
        }
        else if( packet->dns.header->qr == QUERY )
        {
            uint16_t qtype = ntohs(packet->dns.query->qtype);
            if( !(qtype == A || qtype == AAAA) ) // Forward all queries that are not A & AAAA
            {
                ESP_LOGI(TAG, "Forwarding question for %.*s", url.length, url.string);
                forward_query(packet);
                log_query(url, false, packet->src.sin_addr.s_addr);
            }
            else 
            {
                if( memcmp(url.string, device_url, url.length) == 0 ) // Check is qname matches current device url
                {
                    ESP_LOGW(TAG, "Capturing DNS request %.*s", url.length, url.string);
                    capture_query(packet);
                    log_query(url, false, packet->src.sin_addr.s_addr);
                }
                else if( sett::read_bool(sett::BLOCK) && in_blacklist(url) ) // check if url is in blacklist
                {
                    ESP_LOGW(TAG, "Blocking question for %.*s", url.length, url.string);
                    block_query(packet);
                    log_query(url, true, packet->src.sin_addr.s_addr);
                }
                else
                {
                    ESP_LOGD(TAG, "Forwarding question for %.*s", url.length, url.string);
                    forward_query(packet);
                    log_query(url, false, packet->src.sin_addr.s_addr);
                }
            }
            
        }
        int64_t end = esp_timer_get_time();
        ESP_LOGD(TAG, "Processing Time: %lld ms", (end-packet->recv_timestamp)/1000);
    }
}

void start_dns()
{
    ESP_LOGI(TAG, "Initializing DNS...");
    packet_queue = xQueueCreate(PACKET_QUEUE_SIZE, sizeof(Packet*));
    if( packet_queue == NULL )
    {
        THROWE(ESP_ERR_NO_MEM, "Error creating packet queue")
    }

    // read url
    std::string url = sett::read_str(sett::HOSTNAME);
    strcpy(device_url, url.c_str());
    ESP_LOGI(TAG, "Device URL: %s", device_url);

    // read upstream dns
    std::string ip = sett::read_str(sett::DNS_SRV);
    ip4addr_aton(ip.c_str(), (ip4_addr_t *)&upstream_dns.sin_addr.s_addr);
    upstream_dns.sin_family = PF_INET;
    upstream_dns.sin_port = htons(DNS_PORT);
    ESP_LOGI(TAG, "Upstream DNS: %s", inet_ntoa(upstream_dns.sin_addr.s_addr));

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

    BaseType_t xErr = xTaskCreatePinnedToCore(listening_t, "listening_task", 8000, NULL, 9, &listening, 0);
    xErr &= xTaskCreatePinnedToCore(dns_t, "dns_task", 15000, NULL, 9, &dns, 0);
    if( xErr != pdPASS )
    {
        THROWE(DNS_ERR_INIT, "Failed to start dns tasks");
    }
}