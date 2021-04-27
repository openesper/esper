#ifndef IP_H
#define IP_H

#include "esp_system.h"

/**
  * @brief Initialize available interfaces and set provisioning status
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t init_interfaces();

/**
  * @brief Start initialized interfaces in station mode
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t start_interfaces();

/**
  * @brief Initialize TCP/IP stack & event loop
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t init_tcpip();

#endif