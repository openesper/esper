#include "error.h"
#include "string.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "ERROR";

void log_error(int err, const char* error_str, const char* function, const char* file)
{
    if( err < 1 )
    {
        ESP_LOGE(TAG, "%s (%d, %s) in %s", error_str, err, esp_err_to_name(err), function);
    }
    else if( err < 0x100 )
    {
        ESP_LOGE(TAG, "%s (%d, %s) in %s", error_str, err, strerror(err), function);
    }
    else if ( 0x100 < err && err < 0x200 )
    {
        ESP_LOGE(TAG, "%s (%x, %s) in %s", error_str, err, esp_err_to_name(err), function);
    }
    else if ( err < 0x3000 )
    {
        ESP_LOGE(TAG, "%s (%d, %s) in %s", error_str, err, esp_err_to_name(err), function);
    }
}