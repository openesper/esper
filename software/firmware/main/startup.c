#include "error.h"
#include "events.h"
#include "flash.h"
#include "gpio.h"
#include "ip.h"
#include "webserver.h"
#include "url.h"
#include "logging.h"
#include "datetime.h"
#include "dns.h"
#include "ota.h"

#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

static const char *TAG = "APP_BOOT";

// Simple macro that will rollback to previous version if any of the initialization steps fail
#define CHECK(x) if( x != ESP_OK ) rollback();


/**
 * @brief Sets logging levels of different tasks
 *
 * Controls logging levels, used to suppress some logs are startup
 */
esp_err_t set_logging_levels()
{
    esp_log_level_set("heap_init", ESP_LOG_ERROR);
    // esp_log_level_set("spi_flash", ESP_LOG_ERROR);
    esp_log_level_set("wifi", ESP_LOG_ERROR);
    esp_log_level_set("wifi_init", ESP_LOG_ERROR);
    // esp_log_level_set("phy", ESP_LOG_ERROR); 
    // esp_log_level_set("esp_netif_handlers", ESP_LOG_ERROR);
    esp_log_level_set("system_api", ESP_LOG_ERROR);
    esp_log_level_set("esp_eth.netif.glue", ESP_LOG_ERROR);

    return ESP_OK;
}

/**
 * @brief Task outputs memory consumption every 5 seconds
 *
 * Shows every running task, it's stack use, and priority, as well as the total remaining heap size
 */
void task_stacks(void* params)
{
    ESP_LOGI(TAG, "Starting Task Task");
    char buf[1000];
    while(1)
    {
        vTaskDelay( 5000/portTICK_PERIOD_MS );
        ESP_LOGI(TAG, "Min Free Heap: %d", xPortGetMinimumEverFreeHeapSize());
        ESP_LOGI(TAG, "Running Tasks: %d", uxTaskGetNumberOfTasks());

        vTaskList(buf);
        ESP_LOGI(TAG, "\nName\t\tState\tPriority\tStack\tNum\n%s", buf);
    }
}

/**
 * @brief Initialize everything needed to start Esper
 *
 * Initializes ethernet & wifi, pulls blocklist from flash to ram, starts logging task,
 * starts webserver, and then DNS task
 */
esp_err_t initialize()
{
    ESP_LOGI(TAG, "Initializing...");
    ERROR_CHECK(set_bit(INITIALIZING_BIT))
    ERROR_CHECK(initialize_interfaces())
    ERROR_CHECK(initialize_blocklists())
    ERROR_CHECK(initialize_logging())
    ERROR_CHECK(start_webserver())
    ERROR_CHECK(start_dns())
    ERROR_CHECK(clear_bit(INITIALIZING_BIT))

    return ESP_OK;
}

/**
 * @brief Start provisioning service
 *
 * Used to get wifi ssid & password if wifi is enabled.
 * Ethernet is disabled while in this mode, webserver serves a different website, and
 * all DNS queries are captured
 */
esp_err_t start_provisioning()
{
    ESP_LOGI(TAG, "Starting provisioning...");
    ERROR_CHECK(set_bit(PROVISIONING_BIT))
    ERROR_CHECK(turn_on_accesspoint())
    ERROR_CHECK(start_provisioning_webserver())

    ERROR_CHECK(wait_for(PROVISIONED_BIT, portMAX_DELAY))
    ERROR_CHECK(clear_bit(PROVISIONING_BIT))
    ERROR_CHECK(stop_provisioning_webserver())
    ERROR_CHECK(turn_off_accesspoint())

    return ESP_OK;
}

/**
 * @brief Initialize everything needed to run main application
 *
 * Starts Ethernet & Wifi, waits for an IP, then starts SNTP, update checking, and web server
 */
esp_err_t start_application()
{
    ESP_LOGI(TAG, "Starting application...");
    ERROR_CHECK(start_interfaces())
    ERROR_CHECK(wait_for(CONNECTED_BIT, portMAX_DELAY))

    ERROR_CHECK(initialize_sntp())
    ERROR_CHECK(start_update_checking_task())
    ERROR_CHECK(start_application_webserver())

    return ESP_OK;
}

/**
 * @brief Cancel update rollback
 *
 * Marks the currently running running image as valid
 * Currently running image will be run next time the device boots
 */
void cancel_rollback()
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }
}

/**
 * @brief Rollback to previous version
 *
 * Marks the currently running running image as invalid
 * Next time the device boots it will run the previous firmware
 */
void rollback()
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
    }
}

/**
 * @brief Main method
 *
 * Initializes everything, then starts provisioning if not provisioned yet
 * If already provisioned, it starts the main application.
 * 
 * If any of the methods do not return ESP_OK, the firmware image will be marked invalid
 * and not be used again.
 */
void app_main()
{
    set_logging_levels();
    CHECK(initialize_flash())
    CHECK(initialize_gpio())
    CHECK(init_event_group())
    CHECK(initialize())

    if( !check_bit(PROVISIONED_BIT) )
    {
        CHECK(start_provisioning())
    }

    CHECK(start_application())
    cancel_rollback();
}