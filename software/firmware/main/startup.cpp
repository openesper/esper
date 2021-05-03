#include "error.h"
#include "events.h"
#include "flash.h"
#include "filesystem.h"
#include "settings.h"
#include "gpio.h"
#include "ip.h"
#include "logging.h"
#include "dns.h"
#include "webserver.h"
#include "datetime.h"
#include "ota.h"

#include <string>

extern "C" {
    void app_main();
}

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "BOOT";

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
    esp_log_level_set("esp_image", ESP_LOG_ERROR);

    return ESP_OK;
}

void app_main()
{
    try{
        ESP_LOGI(TAG, "Booting...");
        set_logging_levels();
        CHECK(init_event_group())
        set_bit(INITIALIZING_BIT);

        CHECK(init_nvs())
        CHECK(init_gpio())
        fs::init();

        CHECK(init_tcpip())
        CHECK(init_interfaces())

        CHECK(initialize_logging())
        CHECK(start_dns())

        CHECK(start_interfaces())
        CHECK(start_webserver())
        CHECK(init_ota())

        cancel_rollback();
        clear_bit(INITIALIZING_BIT);
    } catch(std::string e){
        rollback();
    }

    wait_for(ETH_CONNECTED_BIT | WIFI_CONNECTED_BIT, portMAX_DELAY);
    try{
        check_for_update();
    }
    catch( std::string e)
    {
        ESP_LOGE(TAG, "Error during OTA: %s", e.c_str());
    }
}