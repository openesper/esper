#ifndef GPIO_H
#define GPIO_H

#include <esp_system.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
  * @brief Get handle for LED task
  *
  * @return task handle for LED task
  */
TaskHandle_t getLEDTaskHandle();

/**
  * @brief Initialize GPIO
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to set update url in flash
  */
esp_err_t initialize_gpio();

#endif