#ifndef GPIO_H
#define GPIO_H

#include <esp_system.h>

// Define common colors
#define OFF         0,0,0
#define PURPLE      148,0,211
#define BLUE        0,0,255
#define WEAK_BLUE   0,0,60
#define GREEN       0,255,0
#define RED         255,0,0
#define YELLOW      255,255,0

// Define color codes
#define INITIALIZING    PURPLE
#define BLOCKING        BLUE
#define NOT_BLOCKING    WEAK_BLUE
#define PROVISIONING    GREEN
#define ERROR           RED
#define OTA             YELLOW

/**
  * @brief Initialize GPIO
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to set update url in flash
  */
void init_gpio();


#endif