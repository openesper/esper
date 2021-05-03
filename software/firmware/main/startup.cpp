#include "error.h"
#include "events.h"
#include "filesystem.h"
#include "settings.h"
#include "gpio.h"
#include "ip.h"
#include "logging.h"
#include "dns.h"
#include "webserver.h"
#include "datetime.h"
#include "ota.h"

#include "nvs_flash.h"

#include <string>

extern "C" {
    void app_main();
}

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "BOOT";

// Simple macro that will rollback to previous version if any of the initialization steps fail
#define CHECK(x) if( x != ESP_OK ) rollback();

void set_logging_levels()
{
    esp_log_level_set("heap_init", ESP_LOG_ERROR);
    esp_log_level_set("wifi", ESP_LOG_ERROR);
    esp_log_level_set("wifi_init", ESP_LOG_ERROR);
    esp_log_level_set("system_api", ESP_LOG_ERROR);
    esp_log_level_set("esp_eth.netif.glue", ESP_LOG_ERROR);
    esp_log_level_set("esp_image", ESP_LOG_ERROR);
}

void init_event_bits(nvs_handle nvs)
{
    set_bit(INITIALIZING_BIT);

    bool eth = false;
    nvs_get_u8(nvs, "ethernet", (uint8_t*)&eth);
    if( eth )
    {
        ESP_LOGI(TAG, "Ethernet enabled");
        set_bit(ETH_ENABLED_BIT);
    }

    bool wifi = true;
    nvs_get_u8(nvs, "wifi", (uint8_t*)&wifi);
    if( wifi )
    {
        ESP_LOGI(TAG, "Wifi enabled");
        set_bit(WIFI_ENABLED_BIT);
    }

    bool gpio = false;
    nvs_get_u8(nvs, "gpio", (uint8_t*)&gpio);
    if( gpio )
    {
        ESP_LOGI(TAG, "GPIO enabled");
        set_bit(GPIO_ENABLED_BIT);
    }
}

static inline void verify_key(nvs_handle nvs, const char* key, uint8_t value)
{
    uint8_t buf = 0;                                                   
    esp_err_t status = nvs_get_u8(nvs, key, &buf);              
    if( status == ESP_ERR_NVS_NOT_FOUND )                           
    {                                                               
        ESP_LOGW(TAG, "%s not found, setting to %d", key, value);   
        status = nvs_set_u8(nvs, key, value);           
    }

    if( status != ESP_OK )                                     
    {                                                               
        THROWE(status, "Error verifying %s", key)                                              
    }                                                               
}

static esp_err_t verify_nvs(nvs_handle nvs)
{
    #ifdef CONFIG_GPIO_ENABLE
    verify_key(nvs, "gpio", CONFIG_GPIO_ENABLE);
    verify_key(nvs, "button", CONFIG_BUTTON);
    verify_key(nvs, "red_led", CONFIG_RED_LED);
    verify_key(nvs, "green_led", CONFIG_GREEN_LED);
    verify_key(nvs, "blue_led", CONFIG_BLUE_LED);
#endif
#ifdef CONFIG_ETHERNET_ENABLE
    verify_key(nvs, "ethernet", CONFIG_ETHERNET_ENABLE);
    verify_key(nvs, "phy_addr", CONFIG_ETH_PHY_ADDR);
    verify_key(nvs, "phy_rst", CONFIG_ETH_PHY_RST_GPIO);
    verify_key(nvs, "phy_mdio", CONFIG_ETH_MDIO_GPIO);
    verify_key(nvs, "phy_mdc", CONFIG_ETH_MDC_GPIO);
    #ifdef CONFIG_ETH_PHY_LAN8720
        verify_key(nvs, "phy", CONFIG_ETH_PHY_LAN8720);
    #elif CONFIG_ETH_PHY_IP101
        verify_key(nvs, "phy", CONFIG_ETH_PHY_IP101);
    #elif CONFIG_ETH_PHY_RTL8201
        verify_key(nvs, "phy", CONFIG_ETH_PHY_RTL8201);
    #elif CONFIG_ETH_PHY_DP83848
        verify_key(nvs, "phy", CONFIG_ETH_PHY_DP83848);
    #endif
#endif
#ifdef CONFIG_WIFI_ENABLE
    verify_key(nvs, "wifi", CONFIG_WIFI_ENABLE);
#endif
    nvs_commit(nvs);

    return ESP_OK;
}

void init_nvs()
{
    ESP_LOGI(TAG, "Initializing NVS...");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGE(TAG, "Error initializing flash, retrying..");
        err = nvs_flash_erase();
        if (err == ESP_OK){
            err = nvs_flash_init();
        }
    }
    if (err != ESP_OK){
        THROWE(err, "Failed to initialize nvs");
    }

    nvs_handle nvs;
    if ( (err = nvs_open("storage", NVS_READWRITE, &nvs)) != ESP_OK )
    {
        THROWE(err, "Failed to open nvs");
    }

    verify_nvs(nvs);
    init_event_bits(nvs);
}

void app_main()
{
    ESP_LOGI(TAG, "Booting...");
    set_logging_levels();

    try{
        init_event_group();
        init_nvs();
        init_gpio();
        init_fs();
        init_interfaces();

        CHECK(initialize_logging())
        start_dns();

        start_interfaces();
        CHECK(start_webserver())
        CHECK(init_ota())
    } catch(const Err& e){
        rollback();
    }

    cancel_rollback();
    clear_bit(INITIALIZING_BIT);
    wait_for(ETH_CONNECTED_BIT | WIFI_CONNECTED_BIT, portMAX_DELAY);
    try{
        check_for_update();
    }
    catch( std::string e )
    {
        ESP_LOGE(TAG, "Error during OTA: %s", e.c_str());
    }
}