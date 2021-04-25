#ifndef WIFI_H
#define WIFI_H

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"

#define MAX_SCAN_RECORDS 5

/**
  * @brief Initialize wifi station netif
  * 
  * @param sta_netif pointer to pointer of esp_netif_t to be filled with netif structure
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t init_wifi_sta_netif(esp_netif_t** sta_netif);

/**
  * @brief Initialize wifi accesspoint netif
  * 
  * @param sta_netif pointer to pointer of esp_netif_t to be filled with netif structure
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t init_wifi_ap_netif(esp_netif_t** ap_netif);

/**
  * @brief Initialize wifi events and allocate resources for wifi
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t configure_wifi();

/**
  * @brief Start a wifi scan, non-blocking
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t wifi_scan();

/**
  * @brief Attempt to connect to configured AP
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t attempt_to_connect(char* ssid, char* pass, bool* result);

#endif