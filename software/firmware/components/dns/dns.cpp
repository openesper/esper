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

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "DNS";

#define PACKET_QUEUE_SIZE 10
#define CLIENT_QUEUE_SIZE 50

static int dns_srv_sock;                                // Socket handle for sending queries to upstream DNS 
static struct sockaddr_in upstream_dns;                 // sockaddr struct containing current upstream server IP
static TaskHandle_t dns;                                // Handle for DNS task
static TaskHandle_t listening;                          // Handle for listening task
static QueueHandle_t packet_queue;                      // FreeRTOS queue of DNS query packets
static Client client_queue[CLIENT_QUEUE_SIZE];          // FIFO Array of clients waiting for DNS response
static char device_url[MAX_URL_LENGTH];                 // Buffer to store current URL in RAM


IRAM_ATTR DNS::DNS(uint8_t* buf, size_t size, sockaddr_in addr_, socklen_t addrlen_)
: addr(addr_), addrlen(addrlen_)
{
    recv_timestamp = esp_timer_get_time();
    buffer.assign(buf, buf+size);
    parse_buffer();
}

IRAM_ATTR DNS::~DNS()
{
    delete question;
    delete[] records;
}

IRAM_ATTR esp_err_t DNS::parse_buffer()
{
    ESP_LOG_BUFFER_HEXDUMP(TAG, buffer.data(), buffer.size(), ESP_LOG_VERBOSE);

    // Cast header to beginning of buffer
    header = (Header*)buffer.data();
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
    ESP_LOGD(TAG, "ARCOUNT (%.4X)\n", additional);

    // point to question in buffer
    question            = new Question; // Multiple questions in one packet is in RFC, but isn't widely supported
    question->qname     = (uint8_t*)header + sizeof(Header);  // qname is immediatly after header
    question->qname_len = strlen((char*)question->qname) + 1;  // qname is \0 terminated so strlen will work
    question->qdata     = (QData*)(question->qname + question->qname_len); // qdata is immediatly after qname
    question->end       = (uint8_t*)question->qdata + sizeof(QData);

    if( question->qname_len > MAX_URL_LENGTH)
    {
        THROWE(DNS_ERR_INVALID_QNAME, "QNAME too large");
    }

    ESP_LOGD(TAG, "Question: %p", question->qname);
    ESP_LOGD(TAG, "QNAME   (%s)(%d)", (char*)(question->qname), question->qname_len);
    ESP_LOGD(TAG, "QTYPE   (%.4X)", ntohs(question->qdata->qtype));
    ESP_LOGD(TAG, "QCLASS  (%.4X)\n", ntohs(question->qdata->qclass));

    // fill in records
    ESP_LOGD(TAG, "Records:");
    records = new ResourceRecord[total_records];
    if( parse_records(question->end, total_records, records) != ESP_OK )
    {
        THROWE(ESP_FAIL, "Error parsing records");
    }

    return ESP_OK;
}

IRAM_ATTR std::string DNS::convert_qname_url()
{
    char temp[question->qname_len];
    memcpy(temp, question->qname, question->qname_len);
    
    char* i = temp;
    while( *i != '\0')
    {
        size_t jmp = *i;
        *i = '.';
        i += jmp + 1;
    }
    
    std::string domain (temp);
    domain.erase(0,1); // rease first '.'
    return domain;
}

IRAM_ATTR esp_err_t DNS::parse_records(uint8_t* start, size_t record_num, ResourceRecord* record)
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
        ESP_LOGD(TAG, "RDDATA  (%.*X)\n", ntohs(record[i].rrdata->rdlength), *record[i].rddata);
    }

    return ESP_OK;
}

IRAM_ATTR esp_err_t DNS::send(int socket, struct sockaddr_in addr)
{
    socklen_t addrlen = sizeof(addr);
    int sendlen = sendto(socket, buffer.data(), buffer.size(), 0, (struct sockaddr *)&addr, addrlen);

    // Wait ~25ms to resend if out of memory
    while( errno == ENOMEM && sendlen < 1)
    {
        vTaskDelay( 25 / portTICK_PERIOD_MS );
        sendlen = sendto(socket, buffer.data(), buffer.size(), 0, (struct sockaddr *)&addr, addrlen);
    }

    if(sendlen < 1)
    {
        ESP_LOGE(TAG, "Failed to send packet, errno=%s", strerror(errno));
        return ESP_FAIL;
    }

    return ESP_OK;
}

static IRAM_ATTR void listening_t(void* parameters)
{
    ESP_LOGD(TAG, "Listening...");
    static uint8_t buffer[MAX_PACKET_SIZE];
    while(1)
    {
        DNS* packet;
        size_t size;
        sockaddr_in addr;
        socklen_t addrlen;
        size = recvfrom(dns_srv_sock, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrlen);
        if( size < 1 )
        {
            ESP_LOGW(TAG, "Error Receiving Packet");
            continue;
        }

        try{
            packet = new DNS(buffer, size, addr, addrlen);
            ESP_LOGD(TAG, "Received %d Byte Packet from %s", size, inet_ntoa(packet->addr.sin_addr.s_addr));
        }catch(const Err& e){
            delete packet;
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

static IRAM_ATTR esp_err_t forward_query(DNS* packet)
{
    if( packet->send(dns_srv_sock, upstream_dns) == ESP_OK )
    {
        for(int i = CLIENT_QUEUE_SIZE-1; i >= 1; i--) 
        {
            client_queue[i] = client_queue[i-1];
        }

        // Add client to beginning of client queue
        client_queue[0].src_address = packet->addr;
        client_queue[0].id = packet->header->id;
        client_queue[0].response_latency = packet->recv_timestamp;
    }

    return ESP_OK;
}

static IRAM_ATTR esp_err_t forward_answer(DNS* packet)
{
    // Go through client queue, searching for client with matching ID.
    // Do nothing if no client is found
    for(int i = 0; i < CLIENT_QUEUE_SIZE; i++)
    {
        if(packet->header->id == client_queue[i].id)
        {
            ESP_LOGD(TAG, "Forwarding answer to %s", inet_ntoa(client_queue[i].src_address.sin_addr.s_addr));
            packet->send(dns_srv_sock, client_queue[i].src_address);
            break;
        }
    }
    return ESP_OK;
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

        std::string domain = packet->convert_qname_url();
        ESP_LOGD(TAG, "Domain  (%s)",   domain.c_str());

        if( packet->header->qr == ANSWER ) // Forward all answers
        {
            ESP_LOGI(TAG, "Forwarding answer for %s", domain.c_str());
            forward_answer(packet);
        }
        else if( packet->header->qr == QUERY )
        {
            uint16_t qtype = ntohs(packet->question->qdata->qtype);
            if( !(qtype == A || qtype == AAAA) ) // Forward all queries that are not A & AAAA
            {
                ESP_LOGI(TAG, "Forwarding question for %s", domain.c_str());
                forward_query(packet);
                // log_query(url, false, packet->src.sin_addr.s_addr);
            }
            else 
            {
                if( memcmp(domain.c_str(), device_url, domain.size()) == 0 ) // Check is qname matches current device url
                {
                    ESP_LOGW(TAG, "Capturing DNS request %s", domain.c_str());
                    // capture_query(packet);
                    // log_query(url, false, packet->src.sin_addr.s_addr);
                }
                else if( sett::read_bool(sett::BLOCK) && in_blacklist(domain.c_str()) ) // check if url is in blacklist
                {
                    ESP_LOGW(TAG, "Blocking question for %s", domain.c_str());
                    // block_query(packet);
                    // log_query(url, true, packet->src.sin_addr.s_addr);
                }
                else
                {
                    ESP_LOGD(TAG, "Forwarding question for %s", domain.c_str());
                    forward_query(packet);
                    // log_query(url, false, packet->src.sin_addr.s_addr);
                }
            }
            
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