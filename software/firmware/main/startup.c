#include "error.h"
#include "events.h"
#include "flash.h"
#include "filesystem.h"
#include "gpio.h"
#include "ip.h"
#include "webserver.h"
#include "url.h"
#include "logging.h"
#include "datetime.h"
#include "dns.h"
#include "ota.h"

#include "sys/stat.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

static const char *TAG = "APP_BOOT";

// Simple macro that will rollback to previous version if any of the initialization steps fail
#define CHECK(x) if( x != ESP_OK ) rollback();

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

void rollback()
{
    set_bit(ERROR_BIT);
    ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
    }

    ESP_LOGE(TAG, "No apps to revert to, rollback failed");
    vTaskDelay( 5000/portTICK_PERIOD_MS );
    esp_restart();
}

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

void app_main()
{
    ESP_LOGI(TAG, "Booting...");
    set_logging_levels();
    CHECK(init_event_group())
    set_bit(INITIALIZING_BIT);

    CHECK(init_nvs())
    CHECK(init_gpio())
    CHECK(init_filesystem())

    CHECK(init_tcpip())
    CHECK(init_interfaces())

    CHECK(initialize_logging())
    CHECK(start_dns())

    CHECK(start_interfaces())
    CHECK(start_webserver())

    cancel_rollback();
    clear_bit(INITIALIZING_BIT);
    ESP_LOGI(TAG, "Done");
}