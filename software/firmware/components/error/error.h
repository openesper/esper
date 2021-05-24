#ifndef ERROR_H
#define ERROR_H

#include "esp_system.h"
#include "errno.h"

#include <string>
#include <exception>


class Err : public std::exception {
        std::string msg;
        int errcode;
        const char* file;
        const char* func;
        int line;
    public:
        Err(std::string msg_, int errcode_, const char* file_, const char* func_, int line) throw();
        virtual const char* what() const throw();
};

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
#define DNS_ERR_INVALID_QNAME       (DNS_ERR_BASE + 2)     // Invalid qname (too long)
#define DNS_ERR_INIT                (DNS_ERR_BASE + 3)
#define DNS_ERR_RECV                (DNS_ERR_BASE + 4)
#define DNS_ERR_QUEUE               (DNS_ERR_BASE + 5)

#define GPIO_ERR_BASE               0x700
#define GPIO_ERR_INIT               (GPIO_ERR_BASE + 1)    // Failed to initialize button  

#define EVENT_ERR_BASE              0x700
#define EVENT_ERR_INIT              (EVENT_ERR_BASE + 1)    // Failed to initialize event group

#define IP_ERR_BASE                 0x800
#define IP_ERR_INIT                 (IP_ERR_BASE + 1)       // Failed to initializ ip

#define ETH_ERR_BASE                0x900
#define ETH_ERR_INIT                (ETH_ERR_BASE + 1)      // Failed to initialize eth

#define FS_ERR_BASE                 0xA00
#define FS_ERR_INIT                 (FS_ERR_BASE + 1)       // Failed to initializ FS     

#define SETTING_ERR_BASE            0xB00
#define SETTING_ERR_NULL            (SETTING_ERR_BASE + 1)
#define SETTING_ERR_INVALID_KEY     (SETTING_ERR_BASE + 2) 
#define SETTING_ERR_WRONG_TYPE      (SETTING_ERR_BASE + 3)
#define SETTING_ERR_NO_FILE         (SETTING_ERR_BASE + 4)
#define SETTING_ERR_PARSE           (SETTING_ERR_BASE + 5)

#define OTA_ERR_BASE                0xC00
#define OTA_ERR_INIT                (OTA_ERR_BASE + 1)
#define OTA_ERR_INCOMPLETE          (OTA_ERR_BASE + 2)
#define OTA_ERR_TOO_LARGE           (OTA_ERR_BASE + 3)
#define OTA_ERR_READ                (OTA_ERR_BASE + 4)


// Macro to make printf style messages easier while throwing an exception
#define THROWE(err, str, ...) {                                     \
  size_t size = snprintf(NULL, 0, str ,##__VA_ARGS__);               \
  char buf[size+1];                                                 \
  snprintf(buf, size+1, str ,##__VA_ARGS__);                        \
  throw Err(buf, err, __FILE__, __func__, __LINE__);                \
}

#define IF_ERR(func) if( (err = func) != ESP_OK ) 
#define IF_NULL(assignment) if( (assignment) == NULL )
#define TRY(func) { esp_err_t err; IF_ERR(func) THROWE(err, #func) }  // Macro for interfacing with functions that return esp_err_t, converts returned error to thrown exception

/**
  * @brief print error, log to error.txt, and return ESP_FAIL if function does not succeed
  * 
  * @param x function with esp_err_t return type
  */
#define ATTEMPT(func) do {                  \
    int tmp_err;                      \
    if( (tmp_err = func) != ESP_OK ) {                    \
        log_error(tmp_err, #func, __func__, __FILE__, __LINE__);   \
        return ESP_FAIL;                                  \
    }                                                     \
} while(0);


void log_error(int err, const char* msg, const char* function, const char* file, int line);

#endif