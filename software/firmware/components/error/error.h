#ifndef ERROR_H
#define ERROR_H

#include "esp_system.h"
#include "errno.h"

#include <string>
#include <exception>

#define WIFI_ERR_BASE               0x200
#define WIFI_ERR_MODE_NULL          (WIFI_ERR_BASE + 1)     // Wifi does not currently have a mode
#define WIFI_ERR_NULL_NETIF         (WIFI_ERR_BASE + 2)     // Wifi netif is null, needs to be initialized
#define WIFI_ERR_NOT_CONFIGURED     (WIFI_ERR_BASE + 3)     // Wifi has not been configured yet
#define WIFI_ERR_INIT               (WIFI_ERR_BASE + 3)     

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

#define DNS_ERR_BASE                0x600
#define DNS_ERR_SOCKET_INIT         (DNS_ERR_BASE + 1)     // Error initialzing socket
#define DNS_ERR_INVALID_QNAME       (DNS_ERR_BASE + 2)     // Invalid qname (too long)

#define GPIO_ERR_BASE               0x700
#define GPIO_ERR_INIT               (GPIO_ERR_BASE + 1)    // Failed to initialize button  

#define EVENT_ERR_BASE              0x700
#define EVENT_ERR_INIT              (EVENT_ERR_BASE + 1)    // Failed to initialize event group

#define IP_ERR_BASE                 0x800
#define IP_ERR_INIT                 (IP_ERR_BASE + 1)       // Failed to initializ ip

#define ETH_ERR_BASE                0x900
#define ETH_ERR_INIT                (ETH_ERR_BASE + 1)      // Failed to initialize eth

#define FS_ERR_BASE                 0xA00     

#define SETTING_ERR_BASE            0xA00
#define SETTING_ERR_NULL            (SETTING_ERR_BASE + 1)
#define SETTING_ERR_INVALID_KEY     (SETTING_ERR_BASE + 2) 
#define SETTING_ERR_WRONG_TYPE      (SETTING_ERR_BASE + 3)          


#define THROW(str, ...) {                                           \
  ESP_LOGE(TAG, str,##__VA_ARGS__);                                 \
  const char* format = str;                                         \
  size_t size = snprintf(NULL, 0, str,##__VA_ARGS__);               \
  char buf[size+1];                                                 \
  snprintf(buf, size, format,##__VA_ARGS__);                        \
  throw std::string(buf);                                           \
}

class esp_exception : public std::exception {
        const char* func;
        const char* file;
        int line;
        std::string msg;
        int errcode;
    public:
        esp_exception(std::string msg_, int errcode_, const char* file_, const char* func_, int line) throw();
        virtual const char* what() const throw();
};

#define STRING(x) #x

/**
  * @brief print error, log to error.txt, and return ESP_FAIL if function does not succeed
  * 
  * @param x function with esp_err_t return type
  */
#define ATTEMPT(func) do {                  \
    int tmp_err;                      \
    if( (tmp_err = func) != ESP_OK ) {                    \
        log_error(tmp_err, STRING(func), __func__, __FILE__);   \
        return ESP_FAIL;                                  \
    }                                                     \
} while(0);


void log_error(int err, const char* error_str, const char* function, const char* file);

#endif