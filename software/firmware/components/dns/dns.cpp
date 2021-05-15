#include "dns/dns.h"

#include <stdexcept>

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
#include "esp_log.h"
static const char *TAG = "DNS";


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

    answer.clss = 1;
    answer.ttl = 128;

    if( question.qtype == A )
    {
        answer.type = A;
        uint32_t ip = inet_addr(ip_str);
        answer.rdlength = sizeof(ip);
        answer.rddata.resize(answer.rdlength);
        memcpy(answer.rddata.data(), &ip, answer.rdlength);
    }
    else if( question.qtype == AAAA )
    {
        answer.type = AAAA;
        struct in6_addr ip;
        inet_pton(AF_INET6, ip_str, &ip);
        answer.rdlength = sizeof(ip.s6_addr);
        answer.rddata.resize(answer.rdlength);
        memcpy(answer.rddata.data(), &ip.s6_addr, answer.rdlength);
    }

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

