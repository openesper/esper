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
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
    }
}

void app_main()
{
    ESP_LOGI(TAG, "Starting Application");
    set_logging_levels();

    CHECK(init_nvs())
    CHECK(init_spiffs())
    CHECK(init_event_group())
    CHECK(initialize_event_bits())
    set_bit(INITIALIZING_BIT);

    CHECK(init_gpio())
    CHECK(init_tcpip())
    CHECK(init_interfaces())
    CHECK(start_interfaces())
    CHECK(initialize_logging())
    CHECK(initialize_blocklists())
    CHECK(start_dns())

    ESP_LOGI(TAG, "Done");
}
    // CHECK(initialize_flash())
    // CHECK(initialize_gpio())
    // CHECK(init_event_group())
    // set_bit(INITIALIZING_BIT);
    // CHECK(initialize())

    // if( !check_bit(PROVISIONED_BIT) )
    // {
    //     CHECK(start_provisioning())
    // }

    // CHECK(start_application())
    // clear_bit(INITIALIZING_BIT);
    // cancel_rollback();


// esp_err_t initialize()
// {
//     ESP_LOGI(TAG, "Initializing...");
//     ERROR_CHECK(initialize_interfaces())
//     ERROR_CHECK(initialize_blocklists())
//     ERROR_CHECK(initialize_logging())
//     ERROR_CHECK(start_webserver())
//     ERROR_CHECK(start_dns())

//     return ESP_OK;
// }


// esp_err_t start_provisioning()
// {
//     ESP_LOGI(TAG, "Starting provisioning...");
//     ERROR_CHECK(set_bit(PROVISIONING_BIT))
//     ERROR_CHECK(turn_on_accesspoint())
//     ERROR_CHECK(start_provisioning_webserver())

//     ERROR_CHECK(wait_for(PROVISIONED_BIT, portMAX_DELAY))
//     ERROR_CHECK(clear_bit(PROVISIONING_BIT))
//     ERROR_CHECK(stop_provisioning_webserver())
//     ERROR_CHECK(turn_off_accesspoint())

//     return ESP_OK;
// }


// esp_err_t start_application()
// {
//     ESP_LOGI(TAG, "Starting application...");
//     ERROR_CHECK(start_interfaces())
//     ERROR_CHECK(wait_for(CONNECTED_BIT, portMAX_DELAY))

//     ERROR_CHECK(initialize_sntp())
//     ERROR_CHECK(start_update_checking_task())
//     ERROR_CHECK(start_application_webserver())

//     return ESP_OK;
// }


// void cancel_rollback()
// {
//     const esp_partition_t *running = esp_ota_get_running_partition();
//     esp_ota_img_states_t ota_state;
//     if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
//         if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
//             ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
//             esp_ota_mark_app_valid_cancel_rollback();
//         }
//     }
// }


// void rollback()
// {
//     const esp_partition_t *running = esp_ota_get_running_partition();
//     esp_ota_img_states_t ota_state;
//     if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
//         if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
//             ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
//             esp_ota_mark_app_invalid_rollback_and_reboot();
//         }
//     }
// }
