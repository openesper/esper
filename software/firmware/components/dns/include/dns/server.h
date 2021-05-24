#ifndef SERVER_H
#define SERVER_H

#include <esp_system.h>
#include "lwip/sockets.h"

#define DNS_PORT 53
#define MAX_URL_LENGTH 255

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