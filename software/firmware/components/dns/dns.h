#ifndef DNS_H
#define DNS_H

#include <esp_system.h>
#include "lwip/sockets.h"

#define DNS_PORT 53

#define MAX_PACKET_SIZE 512

#define A_RECORD_ANSWER_SIZE 16
#define AAAA_RECORD_ANSWER_SIZE 28

/**
  * @brief structs and enums used to parse DNS packets
  * 
  * Documentation: https://www2.cs.duke.edu/courses/fall16/compsci356/DNS/DNS-primer.pdf
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


/**
  * @brief DNS Header
  * 
  *                                 1  1  1  1  1  1
  *   0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * |                     ID                        |
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * |QR| Opcode |AA|TC|RD|RA| Z      |    RCODE     |
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * |                   QDCOUNT                     |
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * |                   ANCOUNT                     |
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * |                   NSCOUNT                     |
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * |                   ARCOUNT                     |
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * 
  */

typedef struct{
    uint16_t id; // identification number
 
    uint8_t rd :1; // recursion desired
    uint8_t tc :1; // truncated message
    uint8_t aa :1; // authoritive answer
    uint8_t opcode :4; // purpose of message
    uint8_t qr :1; // query/response flag
 
    uint8_t rcode :4; // response code
    uint8_t cd :1; // checking disabled
    uint8_t ad :1; // authenticated data
    uint8_t z :1; // its z! reserved
    uint8_t ra :1; // recursion available
 
    uint16_t q_count; // number of question entries
    uint16_t ans_count; // number of answer entries
    uint16_t auth_count; // number of authority entriesS
    uint16_t add_count; // number of resource entries
} DNS_Header;

/**
  * @brief DNS Query
  * 
  *                                 1  1  1  1  1  1
  *   0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * |                                               |
  * /                   QNAME                       /
  * /                                               /
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * |                   NSCOUNT                     |
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * |                   ARCOUNT                     |
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * 
  */
 
typedef struct{
    uint16_t qtype;
    uint16_t qclass;
} DNS_Query; 


/**
  * @brief DNS Answer
  * 
  *                                 1  1  1  1  1  1
  *   0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * |                                               |
  * /                   Name                        /
  * /                                               /
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * |                   TYPE                        |
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * |                   CLASS                       |
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * |                   TTL                         |
  * |                                               |
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * |                   RDLENGTH                    |
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * /                   QNAME                       /
  * /                                               /
  * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * 
  */

typedef struct __attribute__((__packed__)){
    uint16_t name; // Name can be a 16bit pointer to qname
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t rdlength;
    uint32_t rddata;
} DNS_Answer;

typedef struct {
    DNS_Header* header;
    char* qname;
    DNS_Query* query;
} DNS_Packet;

typedef struct {
    struct sockaddr_in src;
    uint8_t data[MAX_PACKET_SIZE];
    uint32_t length;
    int64_t recv_timestamp;
    DNS_Packet dns;
} Packet;

typedef struct {
    struct sockaddr_in src_address;
	uint16_t id;
    int64_t response_latency;
} Client;

/**
  * @brief Retreive upstream dns server from flash
  * 
  * Stores the current server into static sockaddr_in struct
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get server ip from flash
  */
esp_err_t load_upstream_dns();

/**
  * @brief Start listening and DNS parsing tasks
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to start tasks
  */
esp_err_t start_dns();

/**
  * @brief Retreive device url from flash
  * 
  * Stores the current url in static char buffer
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get url from flash
  */
esp_err_t load_device_url();

#endif