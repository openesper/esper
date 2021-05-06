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

#include <stdexcept>

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
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


IRAM_ATTR std::vector<uint8_t> Question::serialize()
{
    std::vector<uint8_t> v(qname.begin(), qname.end());
    v.push_back(qtype >> 8);
    v.push_back(qtype);
    v.push_back(qclass >> 8);
    v.push_back(qclass);
    return v;
}

IRAM_ATTR std::vector<uint8_t> ResourceRecord::serialize()
{
    std::vector<uint8_t> v;
    v.insert(v.end(), name.begin(), name.end());
    v.push_back(type >> 8);
    v.push_back(type);
    v.push_back(clss >> 8);
    v.push_back(clss);
    v.push_back(ttl >> 24);
    v.push_back(ttl >> 16);
    v.push_back(ttl >> 8);
    v.push_back(ttl);
    v.push_back(rdlength >> 8);
    v.push_back(rdlength);
    v.insert(v.end(), rddata.begin(), rddata.end());
    return v;
}

IRAM_ATTR DNS::DNS(std::vector<uint8_t>* buffer, sockaddr_in addr_, socklen_t addrlen_)
: addr(addr_), addrlen(addrlen_)
{
    recv_timestamp = esp_timer_get_time();
    ESP_LOG_BUFFER_HEXDUMP(TAG, buffer->data(), buffer->size(), ESP_LOG_VERBOSE);
    int cursor = 0;

    if( buffer->size() < sizeof( Header) )
    {
        throw std::out_of_range ("Index out of bounds.");
    }

    memcpy(&header, buffer->data(), sizeof(Header));
    cursor += sizeof(Header);

    ESP_LOGV(TAG, "Header:");
    ESP_LOGV(TAG, "ID      (%.4X)", ntohs(header.id));
    ESP_LOGV(TAG, "QR      (%d)", header.qr);
    ESP_LOGV(TAG, "OpCode  (%X)", header.opcode);
    ESP_LOGV(TAG, "AA      (%X)", header.aa);
    ESP_LOGV(TAG, "TC      (%X)", header.tc);
    ESP_LOGV(TAG, "RD      (%X)", header.rd);
    ESP_LOGV(TAG, "RA      (%X)", header.ra);
    ESP_LOGV(TAG, "Z       (%X)", header.z);
    ESP_LOGV(TAG, "RCODE   (%X)", header.rcode);
    ESP_LOGV(TAG, "QCOUNT  (%.4X)", ntohs(header.qcount));
    ESP_LOGV(TAG, "ANCOUNT (%.4X)", ntohs(header.ancount));
    ESP_LOGV(TAG, "NSCOUNT (%.4X)", ntohs(header.nscount));
    ESP_LOGV(TAG, "ARCOUNT (%.4X)\n", ntohs(header.arcount));

    ESP_LOGV(TAG, "Question:");
    unpack_name(buffer, &cursor, &question.qname);
    ESP_LOGV(TAG, "QNAME   (%.*s)(%d)", question.qname.size(), (char*)question.qname.data(), question.qname.size());
    unpack(buffer, &cursor, &question.qtype);
    ESP_LOGV(TAG, "QTYPE   (%.4X)", ntohs(question.qtype));
    unpack(buffer, &cursor, &question.qclass);
    ESP_LOGV(TAG, "QCLASS  (%.4X)\n", ntohs(question.qclass));

    int record_num = ntohs(header.ancount) + ntohs(header.nscount) + ntohs(header.arcount);
    records.resize(record_num);
    for(int i = 0; i < record_num; i++)
    {
        ESP_LOGV(TAG, "Record %d:",  i);
        unpack_name(buffer, &cursor, &records[i].name);
        ESP_LOGV(TAG, "NAME    (%.*s)(%d)", records[i].name.size(), (char*)records[i].name.data(), records[i].name.size());
        
        unpack(buffer, &cursor, &records[i].type);
        ESP_LOGV(TAG, "TYPE    (%.4X)", records[i].type);

        unpack(buffer, &cursor, &records[i].clss);
        ESP_LOGV(TAG, "CLASS   (%.4X)", records[i].clss);

        unpack(buffer, &cursor, &records[i].ttl);
        ESP_LOGV(TAG, "TTL     (%.8X)", records[i].ttl);

        unpack(buffer, &cursor, &records[i].rdlength);
        ESP_LOGV(TAG, "RDLEN   (%.4X)\n", records[i].rdlength);

        unpack_vector(buffer, &cursor, records[i].rdlength, &records[i].rddata);
        // ESP_LOGV(TAG, "RDDATA  (%.*X)(%d)\n", records[i].rddata.size(), (int*)records[i].rddata.data(), records[i].rddata.size());
    }
}

IRAM_ATTR esp_err_t DNS::unpack_name(std::vector<uint8_t>* buffer, int* index, std::vector<uint8_t>* name)
{
    uint8_t byte = 0;
    do {
        byte = buffer->at(*index);
        name->push_back(byte);
        *index += 1;
    } while ( byte != 0xC0 && byte != 0 );

    if( byte == 0xC0 )
    {
        byte = buffer->at(*index);
        name->push_back(byte);
        *index += 1;
    }

    return ESP_OK;
}

IRAM_ATTR esp_err_t DNS::unpack_vector(std::vector<uint8_t>* buffer, int* index, size_t size, std::vector<uint8_t>* dest)
{
    for( int i = 0; i < size; i++ )
    {
        uint8_t byte = buffer->at(*index);
        dest->push_back(byte);
        *index += 1;
    }

    return ESP_OK;
}

template<typename T>
IRAM_ATTR esp_err_t DNS::unpack(std::vector<uint8_t>* buffer, int* index, T* dest)
{
    for( int i = 0; i < sizeof(T); i++ )
    {
        uint8_t byte = buffer->at(*index);
        *dest = *dest << i*8;
        *dest = *dest && 0x00;
        *dest = *dest | byte;
        *index += 1;
    }

    return ESP_OK;
}

IRAM_ATTR std::string DNS::convert_qname_url()
{
    std::string domain (question.qname.begin(), question.qname.end());

    int i = 0;
    while( domain[i] != '\0')
    {
        size_t jmp = (int)domain[i];
        domain[i] = '.';
        i += jmp + 1;
    }
    
    domain.erase(0,1); // rease first '.'
    return domain;
}

IRAM_ATTR esp_err_t DNS::add_answer(const char* ip_str)
{
    header.qr = 1; // change packet to answer
    header.aa = 1; // respect my authoritah
    header.ancount = htons(ntohs(header.ancount) + 1);
    header.arcount = 0;

    ResourceRecord answer;
    uint16_t name = htons(0xC00C);
    answer.name.resize(sizeof(name));
    memcpy(answer.name.data(), &name, sizeof(name));

    answer.type = 1;
    answer.clss = 1;
    answer.ttl = 128;
    answer.rdlength = 4;

    uint32_t ip = inet_addr(ip_str);
    answer.rddata.resize(sizeof(ip));
    memcpy(answer.rddata.data(), &ip, sizeof(ip));

    records.insert(records.begin(), answer);

    return ESP_OK;
}

IRAM_ATTR esp_err_t DNS::send(int socket, struct sockaddr_in addr)
{
    auto const ptr = reinterpret_cast<uint8_t*>(&header);
    std::vector<uint8_t> buffer(ptr, ptr+sizeof(header));
    ESP_LOGV(TAG, "Header Buffer");
    ESP_LOG_BUFFER_HEXDUMP(TAG, buffer.data(), buffer.size(), ESP_LOG_VERBOSE);

    std::vector<uint8_t> questionbuf = question.serialize();
    buffer.insert(buffer.end(), questionbuf.begin(), questionbuf.end());
    ESP_LOGV(TAG, "Question Buffer");
    ESP_LOG_BUFFER_HEXDUMP(TAG, questionbuf.data(), questionbuf.size(), ESP_LOG_VERBOSE);

    for(int i = 0; i < records.size(); i++)
    {
        std::vector<uint8_t> recordbuf = records.at(i).serialize();
        buffer.insert(buffer.end(), recordbuf.begin(), recordbuf.end());
        ESP_LOGV(TAG, "Record Buffer");
        ESP_LOG_BUFFER_HEXDUMP(TAG, recordbuf.data(), recordbuf.size(), ESP_LOG_VERBOSE);
    }
    ESP_LOGV(TAG, "Sending Buffer");
    ESP_LOG_BUFFER_HEXDUMP(TAG, buffer.data(), buffer.size(), ESP_LOG_VERBOSE);

    socklen_t addrlen = sizeof(addr);
    int sendlen = sendto(socket, buffer.data(), buffer.size(), 0, (struct sockaddr *)&addr, addrlen);

    // Wait ~25ms to resend if out of memory
    int retry = 0;
    while( errno == ENOMEM && sendlen < 1)
    {
        vTaskDelay( 25 / portTICK_PERIOD_MS );
        sendlen = sendto(socket, buffer.data(), buffer.size(), 0, (struct sockaddr *)&addr, addrlen);
        if( retry == 2)
            break;
        retry++;
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
        client_queue[0].id = packet->header.id;
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
        if(packet->header.id == client_queue[i].id)
        {
            ESP_LOGV(TAG, "Forwarding answer to %s", inet_ntoa(client_queue[i].src_address.sin_addr.s_addr));
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

        if( packet->header.qr == ANSWER ) // Forward all answers
        {
            ESP_LOGI(TAG, "Forwarding answer for %s", domain.c_str());
            forward_answer(packet);
        }
        else if( packet->header.qr == QUERY )
        {
            uint16_t qtype = packet->question.qtype;
            if( !(qtype == A || qtype == AAAA) ) // Forward all queries that are not A & AAAA
            {
                ESP_LOGI(TAG, "Forwarding question for %s", domain.c_str());
                forward_query(packet);
                log_query(domain, false, qtype, packet->addr.sin_addr.s_addr);
            }
            else 
            {
                if( memcmp(domain.c_str(), device_url, domain.size()) == 0 ) // Check is qname matches current device url
                {
                    ESP_LOGW(TAG, "Capturing DNS request %s", domain.c_str());
                    std::string ip_str = sett::read_str(sett::IP);
                    packet->records.clear();
                    packet->header.arcount = 0;
                    packet->add_answer(ip_str.c_str());
                    packet->send(dns_srv_sock, packet->addr);
                    log_query(domain, false, qtype, packet->addr.sin_addr.s_addr);
                }
                else if( sett::read_bool(sett::BLOCK) && in_blacklist(domain.c_str()) ) // check if url is in blacklist
                {
                    ESP_LOGW(TAG, "Blocking question for %s", domain.c_str());
                    packet->records.clear();
                    packet->header.arcount = 0;
                    packet->add_answer("0.0.0.0");
                    packet->send(dns_srv_sock, packet->addr);
                    log_query(domain, true, qtype, packet->addr.sin_addr.s_addr);
                }
                else
                {
                    ESP_LOGV(TAG, "Forwarding question for %s", domain.c_str());
                    forward_query(packet);
                    log_query(domain, false, qtype, packet->addr.sin_addr.s_addr);
                }
            }
        }

        int64_t end = esp_timer_get_time();
        ESP_LOGV(TAG, "Processing Time: %lld ms (%lld - %lld)", (end-packet->recv_timestamp)/1000, end, packet->recv_timestamp);
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