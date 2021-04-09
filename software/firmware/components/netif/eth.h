#ifndef ETH_H
#define ETH_H

#include "esp_system.h"
#include "esp_eth.h"
#include "esp_netif.h"

enum Phy {
    NONE,
    LAN8720,
    IP101,
    RTL8201,
    DP83848
};

/**
  * @brief Initialize ethernet netif object
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t init_eth_netif(esp_netif_t** eth_netif);

/**
  * @brief Create eth_handle structure
  *
  * @param eth_handle pointer to structure that will be filled with current eth_handle
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t init_eth_handle(esp_eth_handle_t* eth_handle);

/**
  * @brief Create ethernet event handler
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL Failure
  */
esp_err_t init_eth();

#endif