#include "error.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "ERROR";

void log_error(esp_err_t err, const char* error_str, const char* function, const char* file)
{
    ESP_LOGE(TAG, "%s (%s) in %s", error_str, esp_err_to_name(err), function);
}