#include "datetime.h"
#include "string.h"
#include "time.h"
#include "sys/time.h"
#include "lwip/err.h"
#include "lwip/apps/sntp.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "TIME";

static char strftime_buf[64];

char* get_time_str(time_t time)
{
    struct tm timeinfo;
    localtime_r(&time, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%l:%M:%S%p", &timeinfo);
    return strftime_buf;
}

esp_err_t initialize_sntp()
{
    ESP_LOGI(TAG, "Initializing SNTP");

    /* SNTP operating modes: default is to poll using unicast.
    The mode has to be set before calling sntp_init(). */
    sntp_setoperatingmode(SNTP_OPMODE_POLL);

    // Configure SNTP serveer and initialize
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    // Poll SNTP server until current time is retrieved, 
    time_t now = 0;
    struct tm timeinfo = {};
    do {
        ESP_LOGI(TAG, "Getting time from SNTP server...");

        time(&now); // Get current time
        
        localtime_r(&now, &timeinfo); // Convert current time to tm struct, makes parsing easier

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    } while( timeinfo.tm_year < (2016 - 1900) ); // year will be (1970 - 1900) if time is not set

    // Set Timezone to PST (options for configuring TZ coming soon)
    setenv("TZ", "PST8PDT,M3.2.0/2,M11.1.0", 1);
    tzset();
    ESP_LOGI(TAG, "Current Time: %s", get_time_str(now));

    return ESP_OK;
}