#ifndef ETH_H
#define ETH_H

#include "esp_system.h"
#include "esp_eth.h"
#include "esp_netif.h"

/**
  * @brief Initialize ethernet netif object
  * 
  */
esp_netif_t* init_eth_netif();

/**
  * @brief Create eth_handle structure
  *
  */
esp_eth_handle_t init_eth_handle();


#endif