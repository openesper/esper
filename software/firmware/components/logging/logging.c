#include "logging.h"
#include "flash.h"
#include "datetime.h"
#include "error.h"
#include "sys/unistd.h"
#include "unistd.h"
#include "sys/stat.h"
#include "lwip/inet.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "LOGGING";

#define LOG_QUEUE_SIZE 50
static QueueHandle_t log_queue;
static TaskHandle_t logging;

esp_err_t build_log_json(cJSON* json, uint32_t size, uint32_t page)
{
    // Get current position of log
    uint16_t log_head = 0;
    bool full_flag = false;
    ERROR_CHECK(get_log_data(&log_head, &full_flag))

    // Determine number of log entries
    uint16_t log_entries;
    if (full_flag)
        log_entries = MAX_LOGS;
    else
        log_entries = MAX_LOGS-log_head;
    ESP_LOGD(TAG, "head=%d full=%d size=%d", log_head, full_flag, log_entries);

    // Check that page & size parameters aren't larger than number of entries
    if ( log_entries > 0 && (size*page >= log_entries || size > MAX_LOGS) )
    {
        ESP_LOGW(TAG, "Page & size out of bounds");
        return ESP_FAIL;
    }

    // Create log array object
    cJSON* log_entry_array = cJSON_CreateArray();
    if ( json == NULL || log_entry_array == NULL )
        return ESP_FAIL;
    cJSON_AddItemToObject(json, "log_entries", log_entry_array);

    // Open log file
    FILE* log = fopen("/spiffs/log", "r");
    if (log == NULL)
    {
        ESP_LOGW(TAG, "Unable to open log");
        return ESP_FAIL;
    }

    // Calculate current cursor position and move to position in file
    uint16_t current_log_cursor = (log_head + page*size) % MAX_LOGS;
    fseek(log, current_log_cursor*sizeof(Log_Entry), SEEK_SET);
    for(int i = 0; (i < size) & (i < log_entries); i++)
    {
        // Increment cursor
        if (current_log_cursor == MAX_LOGS)
        {
            if(full_flag)
            {
                current_log_cursor = 0;
                fseek(log, 0, SEEK_SET);
            }
            else
                break;
        }
        ESP_LOGV(TAG, "Log cursor: %d(%d)", current_log_cursor, current_log_cursor*sizeof(Log_Entry));

        // Build json entry
        Log_Entry entry = {0};
        if( fread(&entry, sizeof(entry), 1, log) > 0 )
        {
            cJSON* e = cJSON_CreateObject();
            if ( e != NULL)
            {
                cJSON_AddStringToObject(e, "time", get_time_str(entry.time));
                cJSON_AddStringToObject(e, "url", entry.url.string);
                cJSON_AddStringToObject(e, "client", inet_ntoa(entry.client));
                cJSON_AddBoolToObject(e, "blocked", entry.blocked);
                cJSON_AddItemToArray(log_entry_array, e);
            }
            else
                ESP_LOGW(TAG, "Error adding entry to json array");
        }
        current_log_cursor++;
    }
    fclose(log);

    return ESP_OK;
}

esp_err_t log_query(URL url, bool blocked, uint32_t client)
{
    // Create Log_Entry object using given parameters
    Log_Entry entry = {0};
    time(&entry.time);
    entry.url = url;
    entry.blocked = blocked;
    entry.client = client;

    // Add to queue of logs
    BaseType_t err = xQueueSend(log_queue, &entry, 0);
    if(err == errQUEUE_FULL)
    {
        ESP_LOGW(TAG, "Log queue Full, could not add entry");
    }
    else
    {
        xTaskNotify(logging, 1, eSetValueWithoutOverwrite);
        ESP_LOGV(TAG, "Entry Added to Queue");
    }

    return ESP_OK;
}

static void logging_task(void* args)
{
    ESP_LOGI(TAG, "Logging Task Started");
    while(1)
    {
        // Wait for notification that entry has been added to queue
        xTaskNotifyWait(0, 0xFFFFFFFF, NULL, portMAX_DELAY);
        long start = esp_timer_get_time();

        // Get current position of log
        uint16_t log_head = 0;
        bool full_flag = false;
        get_log_data(&log_head, &full_flag);

        // get number of entries in log queue
        uint8_t entries_in_queue = uxQueueMessagesWaiting(log_queue);

        // Open file
        FILE* log = fopen("/spiffs/log", "r+");
        if( !log )
        {
            ESP_LOGW(TAG, "Error opening log file");
            continue;
        }

        // Iterate through queue, adding each entry
        for(int i = 0; i < entries_in_queue; i++)
        {
            // Get log entry
            Log_Entry entry = {0};
            esp_err_t err = xQueueReceive(log_queue, &entry, portMAX_DELAY);
            if(err == pdFALSE)
            {
                ESP_LOGW(TAG, "Error reading entry %d from log_queue", i);
            }
            else
            {
                // Entries are retrieved from queue FIFO (oldest first) so log_head moves backwards so newest entries are at the front of log
                if(log_head <= 0)  // Reset head position if it is at the end
                {
                    log_head = MAX_LOGS;
                    full_flag = true;
                }

                log_head--;
                ESP_LOGV(TAG, "Adding %.*s to log address %d(%d)", entry.url.length, entry.url.string, log_head, log_head*sizeof(Log_Entry));

                // Move to position on log
                if(fseek(log, log_head*sizeof(Log_Entry), SEEK_SET))
                    ESP_LOGW(TAG, "Could not fseek to %d(%d)", log_head, log_head*sizeof(Log_Entry));

                // Write to log and update log data
                if(fwrite(&entry, sizeof(Log_Entry), 1, log))
                {
                    update_log_data(log_head, full_flag);
                }
                else
                {
                    ESP_LOGW(TAG, "Error adding %.*s to log address %d", entry.url.length, entry.url.string, log_head);
                }
            }
        }
        fclose(log);
 
        ESP_LOGI(TAG, "Time to add log %d entries: %.2lf ms", entries_in_queue, (esp_timer_get_time()-start)/1000.0);
    }
}

esp_err_t create_log_file()
{
    ESP_LOGI(TAG, "Creating log file");
    FILE* log = fopen("/spiffs/log", "w");
    if ( !log )
        return LOG_ERR_LOG_UNAVAILABLE;

    Log_Entry entry = {0};
    for(int i = 0; i < MAX_LOGS; i++)
    {
        fwrite(&entry, sizeof(Log_Entry), 1, log);
    }
    ESP_LOGD(TAG, "Log file size %ld bytes", ftell(log));
    fclose(log);

    ERROR_CHECK(update_log_data(MAX_LOGS, false))

    return ESP_OK;
}

esp_err_t initialize_logging()
{
    log_queue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(Log_Entry));

    xTaskCreatePinnedToCore(logging_task, "logging_task", 9000, NULL, 2, &logging, 1);

    return ESP_OK;
}