#ifndef LOGGING_H
#define LOGGING_H

#include <esp_system.h>
#include <string>

typedef struct {
    time_t time;
    uint16_t type;
    uint32_t client;
    std::string domain;
    bool blocked;
} Log_Entry;


Log_Entry get_entry();
size_t get_log_size();

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
esp_err_t log_query(std::string domain, bool blocked, uint16_t type, uint32_t client);

/**
  * @brief Start logging tasks
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t initialize_logging();


#endif