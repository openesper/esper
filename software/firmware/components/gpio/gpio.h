#ifndef GPIO_H
#define GPIO_H

#include <esp_system.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Define common colors
#define PURPLE      148,0,211
#define BLUE        0,0,255
#define WEAK_BLUE   0,0,60
#define GREEN       0,255,0
#define RED         255,0,0
#define YELLOW      255,255,0

/**
  * @brief Get handle for LED task
  *
  * @return task handle for LED task
  */
TaskHandle_t getLEDTaskHandle();

/**
  * @brief Set LED color
  * 
  * @param red red brightness (0-255)
  * @param green green brightness (0-255)
  * @param blue blue brightness (0-255)
  *
  * @return task handle for LED task
  */
esp_err_t set_rgb(uint8_t red, uint8_t green, uint8_t blue);

/**
  * @brief Initialize GPIO
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to set update url in flash
  */
esp_err_t initialize_gpio();

#endif