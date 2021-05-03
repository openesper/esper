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

typedef struct header{
    uint16_t id;        // identification number
 
    uint8_t rd :1;      // recursion desired
    uint8_t tc :1;      // truncated message
    uint8_t aa :1;      // authoritive answer
    uint8_t opcode :4;  // purpose of message
    uint8_t qr :1;      // query/response flag
 
    uint8_t rcode :4;   // response code
    uint8_t z :3;       // reserved (must be all 0)
    uint8_t ra :1;      // recursion available

    uint16_t qcount;    // number of entries in the question section
    uint16_t ancount;   // number of resource records in the answer section
    uint16_t nscount;   // number of name server resource records in the authority records section
    uint16_t arcount;   // number of resource records in the additional records section
} Header;

typedef struct{
    uint16_t qtype;     // a two octet code which specifies the type of the query
    uint16_t qclass;    // a two octet code that specifies the class of the query
} QData; 

typedef struct question{
    uint8_t* qname;     // a domain name represented as a sequence of labels
    size_t qname_len;
    QData* qdata;
} Question;

typedef struct __attribute__((__packed__)){
    uint16_t type;      // two octets containing one of the RR type codes
    uint16_t cls;       // two octets which specify the class of the data in the RDATA field
    uint32_t ttl;       // a 32 bit unsigned integer that specifies the time interval (in seconds) that the resource record may be cached before it should be discarded
    uint16_t rdlength;  // an unsigned 16 bit integer that specifies the length in octets of the RDATA field
} RRData;

typedef struct resource_record{
    uint8_t* name;      // a domain name to which this resource record pertains
    RRData* info;
    uint8_t* rddata;    // a variable length string of octets that describes the resource
} ResourceRecord;

class DNS {
    public:
        struct sockaddr_in src;
        socklen_t addrlen;
        int64_t recv_timestamp;

        uint8_t buffer[MAX_PACKET_SIZE];
        size_t size;

        Header* header;
        Question* question;
        ResourceRecord rr[];

        DNS();
        esp_err_t parse_buffer();
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