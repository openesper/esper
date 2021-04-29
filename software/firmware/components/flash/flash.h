#ifndef FLASH_H
#define FLASH_H

#include "esp_system.h"
#include "esp_netif.h"

#define GOOGLE_IP "8.8.8.8"
#define CLOUDFARE_IP "1.1.1.1"
#define OPENDNS_IP "208.67.222.222"
#define ADGUARD_IP "94.140.14.14"

#define IP4_STRLEN_MAX 16 // Max string length for IP string
#define HOSTNAME_STRLEN_MAX 255 // Max hostname length


/**
  * @brief Retreive network info (ip, gw, nm) from flash
  *
  * @param info pointer to esp_netif_ip_info_t struct that will be filled
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t get_network_info(esp_netif_ip_info_t* info);

/**
  * @brief Store network info (ip, gw, nm) in flash
  *
  * @param info pointer to esp_netif_ip_info_t struct that will be stored in flash
  *
  * @return
  *     - ESP_OK Success
  *     - ESP_FAIL unable to set info in flash
  */
esp_err_t set_network_info(esp_netif_ip_info_t info);

/**
  * @brief Get PHY hardware configuration 
  *
  * @param phy PHY enum, see netif/eth.h for options
  * @param addr PHY rmii address
  * @param rst PHY reset GPIO number
  * @param mdc PHY mdc GPIO number
  * @param mdio PHY mdio GPIO number
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t get_ethernet_phy_config(uint32_t* phy, uint32_t* addr, uint32_t* rst, uint32_t* mdc, uint32_t* mdio);

/**
  * @brief Get button and LED hardware configuration 
  *
  * @param enabled gpio & button enabled
  * @param button button GPIO number
  * @param red red LED GPIO number
  * @param green green LED GPIO number
  * @param blue blue LED GPIO number
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t get_gpio_config(int* button, int* red, int* green, int* blue);

/**
  * @brief Set provisioning status in flash
  *
  * @param provisioned provisioning status
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t set_provisioning_status(bool provisioned);

esp_err_t init_nvs();

#endif