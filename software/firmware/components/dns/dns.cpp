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

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
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

DNS::~DNS()
{
    delete question;
    delete[] records;
}


esp_err_t DNS::parse_buffer()
{
    ESP_LOG_BUFFER_HEXDUMP(TAG, buffer, size, ESP_LOG_VERBOSE);

    // Cast header to beginning of buffer
    header = (Header*)buffer;
    answers     = ntohs(header->ancount);
    authorities = ntohs(header->nscount);
    additional  = ntohs(header->arcount);
    total_records = answers + authorities + additional;

    ESP_LOGD(TAG, "Header: %p", header);
    ESP_LOGD(TAG, "ID      (%.4X)", ntohs(header->id));
    ESP_LOGD(TAG, "QR      (%d)", header->qr);
    ESP_LOGD(TAG, "OpCode  (%X)", header->opcode);
    ESP_LOGD(TAG, "AA      (%X)", header->aa);
    ESP_LOGD(TAG, "TC      (%X)", header->tc);
    ESP_LOGD(TAG, "RD      (%X)", header->rd);
    ESP_LOGD(TAG, "RA      (%X)", header->ra);
    ESP_LOGD(TAG, "Z       (%X)", header->z);
    ESP_LOGD(TAG, "RCODE   (%X)", header->rcode);
    ESP_LOGD(TAG, "QCOUNT  (%.4X)", ntohs(header->qcount));
    ESP_LOGD(TAG, "ANCOUNT (%.4X)", answers);
    ESP_LOGD(TAG, "NSCOUNT (%.4X)", authorities);
    ESP_LOGD(TAG, "ARCOUNT (%.4X)", additional);

    // point to question in buffer
    question = new Question; // Multiple questions in one packet is in RFC, but isn't widely supported
    question->qname = buffer + sizeof(Header);  // qname is immediatly after header
    question->qname_len = strlen((char*)question->qname) + 1;  // qname is \0 terminated so strlen will work
    question->qdata = (QData*)(question->qname + question->qname_len); // qdata is immediatly after qname
    question->end = (uint8_t*)question->qdata + sizeof(QData);

    if( question->qname_len > MAX_URL_LENGTH)
    {
        ESP_LOGW(TAG, "QNAME too large");
        return DNS_ERR_INVALID_QNAME;
    }

    ESP_LOGD(TAG, "Question: %p", question->qname);
    ESP_LOGD(TAG, "QNAME   (%s)(%d)", (char*)(buffer + sizeof(*header)), question->qname_len);
    ESP_LOGD(TAG, "QTYPE   (%.4X)", ntohs(question->qdata->qtype));
    ESP_LOGD(TAG, "QCLASS  (%.4X)", ntohs(question->qdata->qclass));

    ESP_LOGD(TAG, "Records:");
    records = new ResourceRecord[total_records];
    if( parse_records(question->end, total_records, records) != ESP_OK )
    {
        ESP_LOGW(TAG, "Error parsing records");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t DNS::parse_records(uint8_t* start, size_t record_num, ResourceRecord* record)
{
    for(int i = 0; i < record_num; i++)
    {
        if( i == 0 )
            record[i].name = start;
        else
            record[i].name = record[i-1].end;
    
        if( (uint16_t)*record[i].name && 0xC0 ) // see RFC 1035, 4.1.4 for explanation of message compression
        {
            record[i].name_ptr = true;
            record[i].name_len = 2;
        }
        else
        {
            record[i].name_ptr = false;
            record[i].name_len = strlen((char*)record[i].name) + 1; // name has same format qname
        }

        record[i].rrdata = (RRData*)(record[i].name + record[i].name_len);
        record[i].rddata = (uint8_t*)record[i].rrdata + sizeof(RRData);
        record[i].end = record[i].rddata + ntohs(record[i].rrdata->rdlength);

        if( record[i].name_len > MAX_URL_LENGTH )
            return ESP_FAIL;

        if( ntohs(record[i].rrdata->rdlength) > MAX_PACKET_SIZE )
            return ESP_FAIL;

        ESP_LOGD(TAG, "Record %d: %p",  i, record[i].name);
        ESP_LOGD(TAG, "NAME    (%.*s)(%d)", record[i].name_len, record[i].name, record[i].name_len);
        ESP_LOGD(TAG, "TYPE    (%.4X)", ntohs(record[i].rrdata->type));
        ESP_LOGD(TAG, "CLASS   (%.4X)", ntohs(record[i].rrdata->cls));
        ESP_LOGD(TAG, "TTL     (%.8X)", ntohs(record[i].rrdata->ttl));
        ESP_LOGD(TAG, "RDLEN   (%.4X)", ntohs(record[i].rrdata->rdlength));
        ESP_LOGD(TAG, "RDDATA  (%.*X)", ntohs(record[i].rrdata->rdlength), *record[i].rddata);
    }

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
        delete packet;

        BaseType_t xErr = xQueueReceive(packet_queue, &packet, portMAX_DELAY);

        if(xErr == pdFALSE)
        {
            ESP_LOGW(TAG, "Error receiving from queue");
            continue;
        }

        int64_t end = esp_timer_get_time();
        ESP_LOGD(TAG, "Processing Time: %lld ms (%lld - %lld)", (end-packet->recv_timestamp)/1000, end, packet->recv_timestamp);
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