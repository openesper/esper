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


esp_err_t get_file_initialization(bool* init);
esp_err_t set_file_initialization(bool init);

/**
  * @brief Retreive upstream dns server from flash
  *
  * @param str pre-allocated buffer, at least IP4ADDR_STRLEN_MAX in size, that the string will be sent to.
  * 
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get server ip from flash
  */
esp_err_t get_upstream_dns(char* str);

/**
  * @brief Store upstream dns server in flash
  *
  * @param str string to store in flash
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to set server ip in flash
  */
esp_err_t set_upstream_dns(char* str);

/**
  * @brief Retreive device url from flash
  *
  * @param str pre-allocated buffer, at least HOSTNAME_STRLEN_MAX in size, that the string will be sent to
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get url from flash
  */
esp_err_t get_device_url(char* str);

/**
  * @brief Store device url in flash
  *
  * @param str string to store in flash
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to set device url in flash
  */
esp_err_t set_device_url(char* str);

/**
  * @brief Retreive update url from flash
  *
  * @param str pre-allocated buffer, at least HOSTNAME_STRLEN_MAX in size, that the string will be sent to
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get url from flash
  */
esp_err_t get_update_url(char* str);

/**
  * @brief Store update url in flash
  *
  * @param str string to store in flash
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to set update url in flash
  */
esp_err_t set_update_url(char* str);

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
  * @brief Retreive enabled interfaces
  *
  * @param eth bool pointer to fill with ethernet enabled status
  * 
  * @param wifi bool pointer to fill with wifi enabled status
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
// esp_err_t get_enabled_interfaces(bool* eth, bool* wifi);

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
  * @brief Get provisioning status from flash
  *
  * @param provisioned pointer to bool that will be filled with provisioning status
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t get_provisioning_status(bool* provisioned);

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

/**
  * @brief Get current position & status of log file
  *
  * @param head pointer to fill with location of beginning of log
  * 
  * @param full bool pointer for log_full status
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t get_log_data(uint16_t* head, bool* full);

/**
  * @brief Set position and status of log file
  *
  * @param head position of log
  * 
  * @param full full log status
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t update_log_data(uint16_t head, bool full);

/**
  * @brief Reset initialize bit, device will try provisioning at next startup
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t reset_device();

esp_err_t initialize_event_bits();

esp_err_t init_nvs();

#endif