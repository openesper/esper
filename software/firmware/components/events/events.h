#ifndef EVENTS_H
#define EVENTS_H

#include "esp_system.h"
#include "freertos/FreeRTOS.h"

/**
  * @brief Status bits
  */
extern const int ALL_BITS;  
extern const int INITIALIZING_BIT;          // Device is currently initializing
extern const int ERROR_BIT;                 // Device has detected an error
extern const int BLOCKING_BIT;        
extern const int BLOCKED_QUERY_BIT;         // A Query was blocked

extern const int ETH_ENABLED_BIT;
extern const int ETH_INITIALIZED_BIT;
extern const int ETH_CONNECTED_BIT;
extern const int ETH_GOT_IP_BIT;
extern const int WIFI_ENABLED_BIT;
extern const int WIFI_INITIALIZED_BIT;
extern const int WIFI_CONNECTED_BIT;
extern const int WIFI_GOT_IP_BIT;
extern const int GPIO_ENABLED_BIT;

/**
  * @brief Initialize FreeRTOS event group to keep track of system state
  *
  */
void init_event_group();

/**
  * @brief Set a status bit
  *
  * @param bit One of the status bits
  *
  */
void set_bit(int bit);

/**
  * @brief Clear a status bit
  *
  * @param bit One of the status bits
  *
  */
void clear_bit(int bit);

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
  */
void wait_for(int bit, TickType_t xTicksToWait);

/**
  * @brief Toggle a status bit
  *
  * @param bit One of the status bits
  *
  */
void toggle_bit(int bit);

/**
  * @brief Return all event bits
  *
  * @return uint32_t : Current event group
  */
uint32_t get_bits();

#endif

