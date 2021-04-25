#include "flash.h"
#include "error.h"
#include "events.h"
#include "url.h"
#include "wifi.h"
#include "logging.h"
#include "esp_system.h"
#include "string.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/inet.h"
#include "esp_wifi.h"
#include "cJSON.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "FLASH";

static nvs_handle nvs;


esp_err_t get_network_info(esp_netif_ip_info_t* info)
{
    size_t length = sizeof(*info);
    esp_err_t err = nvs_get_blob(nvs, "network_info", (void*)info, &length);
    if( err != ESP_OK ){
        ESP_LOGW(TAG, "Error getting network info (%s)", esp_err_to_name(err));
        esp_netif_ip_info_t ip = {};
        *info = ip;
    }
    return err;
}

esp_err_t set_network_info(esp_netif_ip_info_t info)
{
    esp_err_t err = nvs_set_blob(nvs, "network_info", (void*)&info, sizeof(info));
    if( err != ESP_OK ){
        ESP_LOGE(TAG, "Error setting network info (%s)", esp_err_to_name(err));
    }
    return err;
}

esp_err_t get_log_data(uint16_t* head, bool* full){
    esp_err_t err = nvs_get_u16(nvs, "log_head", head);
    err |= nvs_get_u8(nvs, "full_flag", (uint8_t*)full);

    if( err != ESP_OK ){
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t update_log_data(uint16_t head, bool full)
{
    esp_err_t err = nvs_set_u16(nvs, "log_head", head);
    err |= nvs_set_u8(nvs, "full_flag", (uint8_t)full);

    if( err != ESP_OK ){
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t get_ethernet_phy_config(uint32_t* phy, uint32_t* addr, uint32_t* rst, uint32_t* mdc, uint32_t* mdio)
{
    ATTEMPT(nvs_get_u32(nvs, "phy", phy))
    ATTEMPT(nvs_get_u32(nvs, "phy_addr", addr))
    ATTEMPT(nvs_get_u32(nvs, "phy_rst", rst))
    ATTEMPT(nvs_get_u32(nvs, "phy_mdc", mdc))
    ATTEMPT(nvs_get_u32(nvs, "phy_mdio", mdio))
    return ESP_OK;
}

esp_err_t get_gpio_config(int* button, int* red, int* green, int* blue)
{
    ATTEMPT(nvs_get_i32(nvs, "button", button))
    ATTEMPT(nvs_get_i32(nvs, "red_led", red))
    ATTEMPT(nvs_get_i32(nvs, "green_led", green))
    ATTEMPT(nvs_get_i32(nvs, "blue_led", blue))

    return ESP_OK;
}

esp_err_t set_provisioning_status(bool provisioned)
{
    ATTEMPT(nvs_set_u8(nvs, "provisioned", (uint8_t)provisioned))
    return ESP_OK;
}

static esp_err_t initialize_event_bits()
{
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

    bool provisioned = false;
    nvs_get_u8(nvs, "provisioned", (uint8_t*)&provisioned);
    if( !provisioned )
    {
        ESP_LOGI(TAG, "Not provisioned");
        set_bit(PROVISIONING_BIT);
    }

    return ESP_OK;
}

esp_err_t init_nvs()
{
    ESP_LOGI(TAG, "Initializing NVS...");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGE(TAG, "Error initializing flash");
        err = nvs_flash_erase();
        if (err == ESP_OK){
            err = nvs_flash_init();
        }
    }
    if (err != ESP_OK){
        return err;
    }

    if ( (err = nvs_open("storage", NVS_READWRITE, &nvs)) != ESP_OK )
        return err;

    ATTEMPT(initialize_event_bits())
    return ESP_OK;
}