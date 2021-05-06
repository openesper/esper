#include "logging.h"
#include "datetime.h"
#include "error.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <stack>

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#endif
#include "esp_log.h"
static const char *TAG = "LOG";

#define LOG_SIZE 100
static std::stack<Log_Entry> log;

Log_Entry get_entry()
{
    Log_Entry entry = log.top();
    log.pop();
    ESP_LOGV(TAG, "Poping %s", entry.domain.c_str());
    return entry;
}

size_t get_log_size()
{
    ESP_LOGV(TAG, "Log Size: %d", log.size());
    return log.size();
}

esp_err_t log_query(std::string domain, bool blocked, uint16_t type, uint32_t client)
{
    Log_Entry entry = {};
    time(&entry.time);
    entry.domain = domain;
    entry.blocked = blocked;
    entry.client = client;

    ESP_LOGV(TAG, "Adding %s", entry.domain.c_str());
    log.push(entry);

    if( log.size() == LOG_SIZE )
        log.pop();

    return ESP_OK;
}