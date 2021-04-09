#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "esp_system.h"

/**
  * @brief Start empty webserver
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t start_webserver();

/**
  * @brief Stop webserver, stops application and provisioning as well
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t stop_webserver();

/**
  * @brief Start application webserver
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t start_application_webserver();

/**
  * @brief Start provisioning webserver
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t start_provisioning_webserver();

/**
  * @brief Stop provisioning webserver
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t stop_provisioning_webserver();

#endif