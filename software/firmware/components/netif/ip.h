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
esp_err_t initialize_interfaces();

/**
  * @brief Set Esper to accesspoint mode
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t turn_on_accesspoint();

/**
  * @brief Turn off accesspoint
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t turn_off_accesspoint();

/**
  * @brief Start initialized interfaces in station mode
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t start_interfaces();

#endif