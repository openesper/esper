#ifndef URL_H
#define URL_H

#include <esp_system.h>
#include "cJSON.h"

#define MAX_BLACKLIST_SIZE 50000
#define MAX_URL_LENGTH 255

typedef struct {
    uint8_t length;
    char string[MAX_URL_LENGTH];
} URL;

/**
  * @brief Add url to blacklist
  *
  * @param URL url structure with url string to be added
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t add_to_blacklist(const char* hostname);

/**
  * @brief Remove URL from blacklist
  *
  * @param URL url to be removed from blacklist
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t remove_from_blacklist(const char* hostname);

/**
  * @brief Pull blacklist from flash and store in RAM
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t initialize_blocklists();

/**
  * @brief Check url validity
  *
  * @param url url to be checked
  *
  * @return
  *     - true URL valid
  *     - false URL invalid
  */
bool valid_url(const char* url);

/**
  * @brief Check to see if URL is in blacklist
  *
  * @param url url to be checked
  *
  * @return
  *     - True In blacklist
  *     - False Not in blacklist
  */
IRAM_ATTR bool in_blacklist(URL url);

/**
  * @brief Convert qname string into URL structure
  * 
  * QNAME is not a standard string, needs to be converted before it
  * can be compared to blacklist
  *
  * @param url url to be checked
  *
  * @return URL structure with URL string
  */
IRAM_ATTR URL convert_qname_to_url(char* qname_ptr);

#endif