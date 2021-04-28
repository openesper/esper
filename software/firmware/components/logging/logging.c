#include "logging.h"
#include "datetime.h"
#include "error.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "LOGGING";

#define LOG_SIZE 100
static Log_Entry log[LOG_SIZE];
static uint8_t head = -1;
static SemaphoreHandle_t log_mutex;


Log_Entry get_entry(uint8_t offset)
{
    Log_Entry entry = {0};
    if( offset >= LOG_SIZE )
        return entry;

    int8_t index = head - offset;
    if( index < 0 )
        index = index + LOG_SIZE;

    ESP_LOGD(TAG, "Reading %d", index);
    xSemaphoreTake(log_mutex, portMAX_DELAY);
    entry = log[index];
    xSemaphoreGive(log_mutex);
    
    return entry;
}

esp_err_t log_query(URL url, bool blocked, uint32_t client)
{
    Log_Entry entry = {0};
    time(&entry.time);
    entry.url = url;
    entry.blocked = blocked;
    entry.client = client;

    if( head == LOG_SIZE-1 )
    {
        head = 0;
    }
    else
    {
        head++;
    }

    ESP_LOGD(TAG, "Adding %s at %d", entry.url.string, head);
    xSemaphoreTake(log_mutex, portMAX_DELAY);
    log[head] = entry;
    xSemaphoreGive(log_mutex);


    return ESP_OK;
}


esp_err_t initialize_logging()
{
    log_mutex = xSemaphoreCreateMutex();
    if( log_mutex == NULL )
    {
        log_error(ESP_FAIL, "xSemaphoreCreateMutex()", __func__, __FILE__);
        return ESP_FAIL;
    }

    return ESP_OK;
}