#include "datetime.h"
#include "error.h"
#include "time.h"
#include "sys/time.h"
#include "lwip/apps/sntp.h"

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
#include "esp_log.h"
static const char *TAG = "TIME";


std::string get_time_str(time_t time)
{
    struct tm timeinfo;
    localtime_r(&time, &timeinfo);

    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%l:%M:%S%p", &timeinfo);
    return std::string(strftime_buf);
}

static void sntp_task(void* parameters)
{
    time_t now = 0;
    struct tm timeinfo = {};
    do {
        ESP_LOGV(TAG, "Getting time from SNTP server...");
        time(&now);
        localtime_r(&now, &timeinfo);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    } while( timeinfo.tm_year < (2016 - 1900) ); // year will be (1970 - 1900) if time is not set

    ESP_LOGI(TAG, "Current Time: %s", get_time_str(now).c_str());

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    vTaskDelete(NULL);
}

void initialize_sntp()
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    setenv("TZ", "PST8PDT,M3.2.0/2,M11.1.0", 1);
    tzset();
    
    if( xTaskCreatePinnedToCore(sntp_task, "SNTP task", 2048, NULL, 0, NULL, 0) == pdFALSE )
    {
        THROWE(ESP_FAIL, "Failed to start sntp task");
    }
}