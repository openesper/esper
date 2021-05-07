#include "debug.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef CONFIG_FREERTOS_USE_TRACE_FACILITY
void init_stack_watcher()
{
    ESP_LOGW(TAG, "Debugging not enabled");
}
#else

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
#include "esp_log.h"
static const char *TAG = "DEBUG";

#define LOG_INTERVAL 5000

void stack_watcher(void* params)
{
    ESP_LOGV(TAG, "Starting Task Task");
    char buf[1000];
    while(1)
    {
        vTaskDelay( LOG_INTERVAL/portTICK_PERIOD_MS );
        ESP_LOGV(TAG, "Min Free Heap: %d", xPortGetMinimumEverFreeHeapSize());
        ESP_LOGV(TAG, "Running Tasks: %d", uxTaskGetNumberOfTasks());

        vTaskList(buf);
        ESP_LOGV(TAG, "\nName\t\tState\tPriority\t\tStack\tNum\tCoreID\n%s", buf);
    }
}

void init_stack_watcher()
{
    xTaskCreatePinnedToCore(stack_watcher, "stack_watcher", 5000, NULL, 0, NULL, 0);
}

#endif

