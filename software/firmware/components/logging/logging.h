#ifndef LOGGING_H
#define LOGGING_H

#include <esp_system.h>
#include "url.h"
#include "cJSON.h"

#define MAX_LOGS 100

typedef struct {
    time_t time;
    uint16_t type;
    uint32_t client;
    URL url;
    bool blocked;
} Log_Entry;

/**
  * @brief Get JSON object of logs
  * 
  * @param json pointer to JSON object that logs will be added to
  *
  * @param size page size, also amount of logs to return
  * 
  * @param page page number, returned logs start at page*size
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to build JSON object
  */
esp_err_t build_log_json(cJSON* json, uint32_t size, uint32_t page);

/**
  * @brief Log file is circular buffer, use this to get current position of log 
  * 
  * @param head pointer to uint16_t variable that will be filled with current position of log
  *
  * @param flag pointer to bool that will be filled with log_full status
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t get_log_head(uint16_t* head, bool* flag);

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
  * @brief Create empty log file of size MAX_LOGS*sizeof(Log_Entry)
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t create_log_file();

/**
  * @brief Start logging tasks
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t initialize_logging();


#endif