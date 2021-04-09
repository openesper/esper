#ifndef ERROR_H
#define ERROR_H

#define STRING(x) #x

/**
  * @brief Return ESP_FAIL if x == NULL
  * 
  * @param x pointer
  */
#define NULL_CHECK(x) if( x == NULL ){ return ESP_FAIL; }

/**
  * @brief print error and return ESP_FAIL if function does not return ESP_OK
  * 
  * @param x function with esp_err_t return type
  */
#define ERROR_CHECK(x) do {                                                                 \
            esp_err_t tmp_err = (x);                                                        \
            if (tmp_err != ESP_OK) {                                                        \
                ESP_LOGE(TAG, STRING(x) " (%s)(%X)", esp_err_to_name(tmp_err), tmp_err);    \
                return ESP_FAIL;                                                            \
            }                                                                               \
        } while(0);


#define WIFI_ERR_BASE               0x200
#define WIFI_ERR_MODE_NULL          (WIFI_ERR_BASE + 1)     // Wifi does not currently have a mode
#define WIFI_ERR_NULL_NETIF         (WIFI_ERR_BASE + 2)     // Wifi netif is null, needs to be initialized
#define WIFI_ERR_NOT_CONFIGURED     (WIFI_ERR_BASE + 3)     // Wifi has not been configured yet

#define URL_ERR_BASE                0x300
#define URL_ERR_TOO_LONG            (URL_ERR_BASE + 1)      // URL string is too long
#define URL_ERR_LIST_UNAVAILBLE     (URL_ERR_BASE + 2)      // Cannot access blacklist
#define URL_ERR_LIST_FUll           (URL_ERR_BASE + 3)      // No free memory in blacklist
#define URL_ERR_NOT_FOUND           (URL_ERR_BASE + 4)      // Can not find URL
#define URL_ERR_ALREADY_EXISTS      (URL_ERR_BASE + 5)      // URL is already in blacklist
#define URL_ERR_INVALID_URL         (URL_ERR_BASE + 6)      // URL contains invalid characters

#define LOG_ERR_BASE                0x400
#define LOG_ERR_LOG_UNAVAILABLE     (LOG_ERR_BASE + 1)      // Cannot access log file

#define IO_ERR_BASE                 0x500
#define IO_ERR_BUTTON_INIT          (IO_ERR_BASE + 1)       // Cannot initialize button
#define IO_ERR_LED_INIT             (IO_ERR_BASE + 2)       // Cannot initialize LEDs

#define DNS_ERR_BASE                 0x600
#define DNS_ERR_SOCKET_INIT          (DNS_ERR_BASE + 1)     // Error initialzing socket
#define DNS_ERR_INVALID_QNAME        (DNS_ERR_BASE + 2)     // Invalid qname (too long)

#endif