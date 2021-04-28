#ifndef LOGGING_H
#define LOGGING_H

#include <esp_system.h>
#include "url.h"

typedef struct {
    time_t time;
    uint16_t type;
    uint32_t client;
    URL url;
    bool blocked;
} Log_Entry;


Log_Entry get_entry(uint8_t offset);

/**
  * @brief Add entry to log
  * 
  * @param url URL structure with string of URL to be added
  *
  * @param blocked boolean of wether the query was blocked
  * 
  * @param client IP of the device that sent query
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t log_query(URL url, bool blocked, uint32_t client);

/**
  * @brief Start logging tasks
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t initialize_logging();


#endif