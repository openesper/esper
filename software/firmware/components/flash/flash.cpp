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

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "FLASH";

static nvs_handle nvs;

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

    return ESP_OK;
}


#define VERIFY_KEY(type, name, key, default) { \
    type buf = 0;                                                   \
    esp_err_t status = nvs_get_##name(nvs, key, &buf);              \
    if( status == ESP_ERR_NVS_NOT_FOUND )                           \
    {                                                               \
        ESP_LOGW(TAG, "%s not found, setting to %d", key, default); \
        ATTEMPT(nvs_set_##name(nvs, key, default))                  \
    }                                                               \
    else if( status != ESP_OK )                                     \
    {                                                               \
        return ESP_FAIL;                                            \
    }                                                               \
}

static esp_err_t verify_nvs()
{
#ifdef CONFIG_GPIO_ENABLE
    VERIFY_KEY(uint8_t,  u8,  "gpio", CONFIG_GPIO_ENABLE)
    VERIFY_KEY(int32_t,  i32, "button", CONFIG_BUTTON)
    VERIFY_KEY(int32_t,  i32, "red_led", CONFIG_RED_LED)
    VERIFY_KEY(int32_t,  i32, "green_led", CONFIG_GREEN_LED)
    VERIFY_KEY(int32_t,  i32, "blue_led", CONFIG_BLUE_LED)
#endif
#ifdef CONFIG_ETHERNET_ENABLE
    VERIFY_KEY(uint8_t,  u8,  "ethernet", CONFIG_ETHERNET_ENABLE)
    VERIFY_KEY(uint32_t, u32, "phy_addr", CONFIG_ETH_PHY_ADDR)
    VERIFY_KEY(uint32_t, u32, "phy_rst", CONFIG_ETH_PHY_RST_GPIO)
    VERIFY_KEY(uint32_t, u32, "phy_mdio", CONFIG_ETH_MDIO_GPIO)
    VERIFY_KEY(uint32_t, u32, "phy_mdc", CONFIG_ETH_MDC_GPIO)
    #ifdef CONFIG_ETH_PHY_LAN8720
        VERIFY_KEY(uint32_t, u32, "phy", CONFIG_ETH_PHY_LAN8720)
    #elif CONFIG_ETH_PHY_IP101
        VERIFY_KEY(uint32_t, u32, "phy", CONFIG_ETH_PHY_IP101)
    #elif CONFIG_ETH_PHY_RTL8201
        VERIFY_KEY(uint32_t, u32, "phy", CONFIG_ETH_PHY_RTL8201)
    #elif CONFIG_ETH_PHY_DP83848
        VERIFY_KEY(uint32_t, u32, "phy", CONFIG_ETH_PHY_DP83848)
    #endif
#endif
#ifdef CONFIG_WIFI_ENABLE
    VERIFY_KEY(uint8_t,  u8,   "wifi", CONFIG_WIFI_ENABLE)
#endif
    VERIFY_KEY(uint8_t,  u8,  "provisioned", 0)

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

    ATTEMPT(verify_nvs())
    ATTEMPT(initialize_event_bits())
    return ESP_OK;
}