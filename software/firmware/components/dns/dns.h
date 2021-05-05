#ifndef DNS_H
#define DNS_H

#include <esp_system.h>
#include "lwip/sockets.h"
#include <string>
#include <vector>

#define DNS_PORT 53

#define MAX_PACKET_SIZE 512
#define MAX_URL_LENGTH 255

#define A_RECORD_ANSWER_SIZE 16
#define AAAA_RECORD_ANSWER_SIZE 28

/**
  * @brief structs and enums used to parse DNS packets
  * 
  * Documentation:
  * https://tools.ietf.org/html/rfc1035
  * https://www.freesoft.org/CIE/RFC/1035/39.htm
  * https://www2.cs.duke.edu/courses/fall16/compsci356/DNS/DNS-primer.pdf
  * 
  */

enum QRFlag {
    QUERY,
    ANSWER
};

enum RecordTypes {
    A=1,
    NS=2,
    AAAA=28,
};

typedef struct header{
    uint16_t id;        // identification number
 
    uint8_t rd :1;      // recursion desired
    uint8_t tc :1;      // truncated message
    uint8_t aa :1;      // authoritive answer
    uint8_t opcode :4;  // purpose of message
    uint8_t qr :1;      // query/response flag
 
    uint8_t rcode :4;   // response code
    uint8_t cd :1;      // checking disabled
    uint8_t ad :1;      // authenticated data
    uint8_t z :1;       // its z! reserved
    uint8_t ra :1;      // recursion available

    uint16_t qcount;    // number of entries in the question section
    uint16_t ancount;   // number of resource records in the answer section
    uint16_t nscount;   // number of name server resource records in the authority records section
    uint16_t arcount;   // number of resource records in the additional records section
} Header;

class Question {
    public:
        std::vector<uint8_t> qname;
        uint16_t qtype;
        uint16_t qclass;
        std::vector<uint8_t> serialize();
};

class ResourceRecord {
    public:
        std::vector<uint8_t> name;
        uint16_t type;
        uint16_t clss;
        uint32_t ttl;
        uint16_t rdlength;
        std::vector<uint8_t> rddata;
        std::vector<uint8_t> serialize();
};

class DNS {
    private:
        IRAM_ATTR esp_err_t parse_buffer(uint8_t* buffer, size_t size);
        IRAM_ATTR esp_err_t unpack_name(std::vector<uint8_t>* buffer, int* index, std::vector<uint8_t>* name);
        IRAM_ATTR esp_err_t unpack_vector(std::vector<uint8_t>* buffer, int* index, size_t size, std::vector<uint8_t>* dest);

        template<typename T>
        IRAM_ATTR esp_err_t unpack(std::vector<uint8_t>* buffer, int* index, T* dest);

    public:
        struct sockaddr_in addr;
        socklen_t addrlen;
        int64_t recv_timestamp;

        Header header;
        Question question;
        std::vector<ResourceRecord> records;

        IRAM_ATTR DNS(std::vector<uint8_t>* buffer, sockaddr_in addr_, socklen_t addrlen_);
        
        IRAM_ATTR std::string convert_qname_url();
        IRAM_ATTR esp_err_t add_answer(const char* ip);
        IRAM_ATTR esp_err_t send(int socket, struct sockaddr_in addr);
};

typedef struct {
    struct sockaddr_in src_address;
	uint16_t id;
    int64_t response_latency;
} Client;

/**
  * @brief Start listening and DNS parsing tasks
  *
  */
void start_dns();

#endif