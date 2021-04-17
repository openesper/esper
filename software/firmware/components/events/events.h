#ifndef EVENTS_H
#define EVENTS_H

#include "esp_system.h"
#include "freertos/FreeRTOS.h"

/**
  * @brief Status bits
  */

extern const int SCAN_FINISHED_BIT;         // Wifi station has finished scanning available IPs
extern const int CONNECTED_BIT;             // Device is connected 
extern const int DISCONNECTED_BIT;          // Device is disconnected
extern const int PROVISIONING_BIT;          // Device is currently in provisioning mode
extern const int PROVISIONED_BIT;           // Device has already been provisioned
extern const int STATIC_IP_BIT;             // Device has received a static IP
extern const int BLOCKING_BIT;              // Device is currently blocking DNS queries
extern const int INITIALIZING_BIT;          // Device is currently initializing
extern const int ERROR_BIT;                 // Device has detected an error
extern const int OTA_BIT;                   // Device is currently undergoing an over-the-air update
extern const int UPDATE_AVAILABLE_BIT;      // Device detected that an update is available
extern const int ETHERNET_CONNECTED_BIT;    // Ethernet is currently connected
extern const int WIFI_CONNECTED_BIT;        // Wifi is currently connected
extern const int BLOCKED_QUERY_BIT;         // A Query was blocked

/**
  * @brief Initialize FreeRTOS event group to keep track of system state
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to create event group
  */
esp_err_t init_event_group();

/**
  * @brief Set a status bit
  *
  * @param bit One of the status bits
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to set bit
  */
esp_err_t set_bit(int bit);

/**
  * @brief Clear a status bit
  *
  * @param bit One of the status bits
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to clear bit
  */
esp_err_t clear_bit(int bit);

/**
  * @brief Check the status of a bit
  *
  * @param bit One of the status bits
  *
  * @return
  *    - true : bit is set
  *    - false: bit is cleared
  */
bool check_bit(int bit);

/**
  * @brief Wait for a bit to be set
  * 
  * Uses FreeRTOS api so calling task does not use
  * CPU time while waiting for bit
  *
  * @param bit One of the status bits
  *
  * @param TickType_t xTicksToWait : time to wait for bit to be set
  * 
  * @return
  *    - ESP_OK bit was set, or time expired
  *    - ESP_FAIL error checking bit
  */
esp_err_t wait_for(int bit, TickType_t xTicksToWait);

/**
  * @brief Toggle a status bit
  *
  * @param bit One of the status bits
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to toggle bit
  */
esp_err_t toggle_bit(int bit);

/**
  * @brief Return all event bits
  *
  * @return uint32_t : Current event group
  */
uint32_t get_bits();

#endif

