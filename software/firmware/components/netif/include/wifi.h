#ifndef WIFI_H
#define WIFI_H

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"

/**
  * @brief Initialize wifi station netif
  * 
  * @param sta_netif pointer to pointer of esp_netif_t to be filled with netif structure
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_netif_t* init_wifi_sta_netif();

#endif