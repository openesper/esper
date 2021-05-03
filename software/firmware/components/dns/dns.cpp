#include "dns.h"
#include "error.h"
#include "events.h"
#include "settings.h"
#include "url.h"
#include "logging.h"

#include "errno.h"
#include "stdio.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "esp_netif.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
static const char *TAG = "DNS";

#define PACKET_QUEUE_SIZE 10
#define CLIENT_QUEUE_SIZE 50

static int dns_srv_sock;                                // Socket handle for sending queries to upstream DNS 
static struct sockaddr_in upstream_dns;                 // sockaddr struct containing current upstream server IP
static TaskHandle_t dns;                                // Handle for DNS task
static TaskHandle_t listening;                          // Handle for listening task
static QueueHandle_t packet_queue;                      // FreeRTOS queue of DNS query packets
// static Client client_queue[CLIENT_QUEUE_SIZE];          // FIFO Array of clients waiting for DNS response
static char device_url[MAX_URL_LENGTH];                 // Buffer to store current URL in RAM


DNS::DNS()
{
    recv_timestamp = esp_timer_get_time();
}

esp_err_t DNS::parse_buffer()
{
    header = (Header*)buffer; // Cast header to beginning of buffer
    ESP_LOGD(TAG, "Header:");
    ESP_LOGD(TAG, "ID      (%d)", header->id);
    ESP_LOGD(TAG, "QR      (%d)", header->qr);
    ESP_LOGV(TAG, "QCOUNT  (%d)", header->qcount);
    ESP_LOGV(TAG, "ANCOUNT (%d)", header->ancount);
    ESP_LOGV(TAG, "NSCOUNT (%d)", header->nscount);
    ESP_LOGV(TAG, "ARCOUNT (%d)", header->arcount);


    question->qname = buffer + sizeof(Header); // qname is immediatly after header
    question->qname_len = strlen((char*)question->qname) + 1; // qname is \0 terminated so strlen will work
    question->qdata = (QData*)question->qname + question->qname_len; // qdata is immediatly after qname

    if( question->qname_len > MAX_URL_LENGTH)
    {
        ESP_LOGW(TAG, "QNAME too large");
        return DNS_ERR_INVALID_QNAME;
    }
    ESP_LOGD(TAG, "QNAME   (%s)", (char*)question->qname);
    ESP_LOGD(TAG, "QTYPE   (%d)", question->qdata->qtype);
    ESP_LOGD(TAG, "QCLASS  (%d)", question->qdata->qclass);

    return ESP_OK;
}

static IRAM_ATTR void listening_t(void* parameters)
{
    ESP_LOGD(TAG, "Listening...");
    while(1)
    {
        DNS* packet = new DNS();
        
        packet->size = recvfrom(dns_srv_sock, packet->buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&(packet->src), &packet->addrlen);
        if( packet->size < 1 )
        {
            ESP_LOGW(TAG, "Error Receiving Packet");
            free(packet);
            continue;
        }
        ESP_LOGD(TAG, "Received %d Byte Packet from %s", packet->size, inet_ntoa(packet->src.sin_addr.s_addr));

        if( packet->parse_buffer() != ESP_OK )
        {
            free(packet);
            continue;
        }
        
        if( xQueueSend(packet_queue, &packet, 0) == errQUEUE_FULL )
        {
            ESP_LOGE(TAG, "Queue Full, could not add packet");
            free(packet);
            continue;
        }
    }
}

static IRAM_ATTR void dns_t(void* parameters)
{
    DNS* packet = NULL;
    while(1) 
    {
        if( packet ) free(packet);

        BaseType_t xErr = xQueueReceive(packet_queue, &packet, portMAX_DELAY);
        ESP_LOGD(TAG, "Read from queue");

        if(xErr == pdFALSE)
        {
            ESP_LOGW(TAG, "Error receiving from queue");
            continue;
        }

        int64_t end = esp_timer_get_time();
        ESP_LOGD(TAG, "Processing Time: %lld ms (%lld - %lld)", (end-packet->recv_timestamp)/1000, end, packet->recv_timestamp);
        // if( parse_packet(packet) != ESP_OK )
        // {
        //     ESP_LOGI(TAG, "Error parsing packet");
        //     continue;
        // }

        // URL url = convert_qname_to_url(packet->dns.qname);
        // if( packet->dns.header->qr == ANSWER ) // Forward all answers
        // {
        //     ESP_LOGI(TAG, "Forwarding answer for %.*s", url.length, url.string);
        //     // forward_answer(packet);
        // }
        // else if( packet->dns.header->qr == QUERY )
        // {
        //     uint16_t qtype = ntohs(packet->dns.query->qtype);
        //     if( !(qtype == A || qtype == AAAA) ) // Forward all queries that are not A & AAAA
        //     {
        //         ESP_LOGI(TAG, "Forwarding question for %.*s", url.length, url.string);
        //         // forward_query(packet);
        //         // log_query(url, false, packet->src.sin_addr.s_addr);
        //     }
        //     else 
        //     {
        //         if( memcmp(url.string, device_url, url.length) == 0 ) // Check is qname matches current device url
        //         {
        //             ESP_LOGW(TAG, "Capturing DNS request %.*s", url.length, url.string);
        //             // capture_query(packet);
        //             // log_query(url, false, packet->src.sin_addr.s_addr);
        //         }
        //         else if( sett::read_bool(sett::BLOCK) && in_blacklist(url) ) // check if url is in blacklist
        //         {
        //             ESP_LOGW(TAG, "Blocking question for %.*s", url.length, url.string);
        //             // block_query(packet);
        //             // log_query(url, true, packet->src.sin_addr.s_addr);
        //         }
        //         else
        //         {
        //             ESP_LOGD(TAG, "Forwarding question for %.*s", url.length, url.string);
        //             // forward_query(packet);
        //             // log_query(url, false, packet->src.sin_addr.s_addr);
        //         }
        //     }
            
        // }
    }
}

void start_dns()
{
    ESP_LOGI(TAG, "Initializing DNS...");
    packet_queue = xQueueCreate(PACKET_QUEUE_SIZE, sizeof(DNS*));
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