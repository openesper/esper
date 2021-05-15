#include "dns/logging.h"
#include "datetime.h"
#include "error.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <vector>

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
#include "esp_log.h"
static const char *TAG = "LOG";

#define LOG_SIZE 100
static std::vector<Log_Entry> log;

Log_Entry get_entry(int index)
{
    Log_Entry entry = log[index];
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
    log.insert(log.begin(), entry);

    if( log.size() == LOG_SIZE )
        log.pop_back();

    return ESP_OK;
}