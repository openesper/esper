#ifndef URL_H
#define URL_H

#include <esp_system.h>
#include "cJSON.h"

#define MAX_BLACKLIST_SIZE 25000
#define MAX_URL_LENGTH 255

typedef struct {
    uint8_t length;
    char string[MAX_URL_LENGTH];
} URL;

/**
  * @brief Get pointer blacklist, takes blacklist mutex lock
  *
  * @param size pointer to int that will be filled with number of entries in blacklist
  * 
  * @return pointer to blacklist buffer
  */
char* get_blacklist(int* size);

/**
  * @brief Return blacklist mutex lock
  */
void return_blacklist();

/**
  * @brief Add url to blacklist
  *
  * @param URL url structure with url string to be added
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t add_to_blacklist(URL url);

/**
  * @brief Remove URL from blacklist
  *
  * @param URL url to be removed from blacklist
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t remove_from_blacklist(URL url);

/**
  * @brief Build json object with urls to be added
  *
  * @param json pointer to JSON to be filled with blacklist
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t build_blacklist_json(cJSON* json);

/**
  * @brief Read default blacklist file and store in flash
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t store_default_blacklists();

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
bool valid_url(URL url);

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