#include "flash.h"
#include "error.h"
#include "url.h"
#include "wifi.h"
#include "logging.h"
#include "esp_system.h"
#include "string.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "esp_netif.h"
#include "lwip/inet.h"
#include "esp_wifi.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "FLASH";

static nvs_handle nvs;

esp_err_t get_upstream_dns(char* str)
{
    size_t length = IP4ADDR_STRLEN_MAX;
    ERROR_CHECK(nvs_get_str(nvs, "upstream_server", str, &length))
    return ESP_OK;
}

esp_err_t set_upstream_dns(char* str)
{
    ERROR_CHECK(nvs_set_str(nvs, "upstream_server", str))
    return ESP_OK;
}

esp_err_t get_device_url(char* str)
{
    size_t length = MAX_URL_LENGTH;
    ERROR_CHECK(nvs_get_str(nvs, "url", str, &length))
    return ESP_OK;
}

esp_err_t set_device_url(char* str)
{
    ERROR_CHECK(nvs_set_str(nvs, "url", str))
    return ESP_OK;
}

esp_err_t get_update_url(char* str)
{
    size_t length = MAX_URL_LENGTH;
    ERROR_CHECK(nvs_get_str(nvs, "update_url", str, &length))
    return ESP_OK;
}

esp_err_t set_update_url(char* str)
{
    ERROR_CHECK(nvs_set_str(nvs, "update_url", str))
    return ESP_OK;
}

esp_err_t get_network_info(esp_netif_ip_info_t* info)
{
    size_t length = sizeof(*info);
    esp_err_t err = nvs_get_blob(nvs, "network_info", (void*)info, &length);
    if( err != ESP_OK ){
        ESP_LOGE(TAG, "Error getting network info (%s)", esp_err_to_name(err));
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

esp_err_t get_enabled_interfaces(bool* eth, bool* wifi)
{
    ERROR_CHECK(nvs_get_u8(nvs, "ethernet", (uint8_t*)eth))
    ERROR_CHECK(nvs_get_u8(nvs, "wifi", (uint8_t*)wifi))
    return ESP_OK;
}

esp_err_t get_ethernet_phy_config(uint32_t* phy, uint32_t* addr, uint32_t* rst, uint32_t* mdc, uint32_t* mdio)
{
    ERROR_CHECK(nvs_get_u32(nvs, "phy", phy))
    ERROR_CHECK(nvs_get_u32(nvs, "phy_addr", addr))
    ERROR_CHECK(nvs_get_u32(nvs, "phy_rst", rst))
    ERROR_CHECK(nvs_get_u32(nvs, "phy_mdc", mdc))
    ERROR_CHECK(nvs_get_u32(nvs, "phy_mdio", mdio))
    return ESP_OK;
}

esp_err_t get_gpio_config(bool* enabled, int* button, int* red, int* green, int* blue)
{
    ERROR_CHECK(nvs_get_i8(nvs, "gpio", (int8_t*)enabled))
    ERROR_CHECK(nvs_get_i32(nvs, "button", button))
    ERROR_CHECK(nvs_get_i32(nvs, "red_led", red))
    ERROR_CHECK(nvs_get_i32(nvs, "green_led", green))
    ERROR_CHECK(nvs_get_i32(nvs, "blue_led", blue))

    return ESP_OK;
}

static esp_err_t init_settings()
{
    ESP_LOGI(TAG, "Saving Settings");
#if CONFIG_UPSTREAM_DNS_GOOGLE
    ERROR_CHECK(nvs_set_str(nvs, "upstream_server", GOOGLE_IP, IP4_STRLEN_MAX))
#elif CONFIG_UPSTREAM_DNS_CLOUDFARE
    ERROR_CHECK(nvs_set_str(nvs, "upstream_server", CLOUDFARE_IP))
#elif CONFIG_UPSTREAM_DNS_OPENDNS
    ERROR_CHECK(nvs_set_str(nvs, "upstream_server", OPENDNS_IP, IP4_STRLEN_MAX))
#elif CONFIG_UPSTREAM_DNS_ADGUARD
    ERROR_CHECK(nvs_set_str(nvs, "upstream_server", ADGUARD_IP, IP4_STRLEN_MAX))
#endif
    ERROR_CHECK(nvs_set_str(nvs, "url", CONFIG_DEFAULT_DEVICE_URL))
    ERROR_CHECK(nvs_set_str(nvs, "update_url", CONFIG_DEFAULT_UPDATE_URI))
    return ESP_OK;
}

static esp_err_t init_interfaces()
{
    ESP_LOGI(TAG, "Saving Interface Configuration");
    
#ifdef CONFIG_ETHERNET_ENABLE
    ERROR_CHECK(nvs_set_u8(nvs, "ethernet", true))
    ERROR_CHECK(nvs_set_u32(nvs, "phy_addr", CONFIG_ETH_PHY_ADDR))
    ERROR_CHECK(nvs_set_u32(nvs, "phy_rst", CONFIG_ETH_PHY_RST_GPIO))
    ERROR_CHECK(nvs_set_u32(nvs, "phy_mdc", CONFIG_ETH_MDC_GPIO))
    ERROR_CHECK(nvs_set_u32(nvs, "phy_mdio", CONFIG_ETH_MDIO_GPIO))
#ifdef CONFIG_ETH_PHY_LAN8720
    ERROR_CHECK(nvs_set_u32(nvs, "phy", 1))
#elif CONFIG_ETH_PHY_IP101
    ERROR_CHECK(nvs_set_u32(nvs, "phy", 2))
#elif CONFIG_ETH_PHY_RTL8201
    ERROR_CHECK(nvs_set_u32(nvs, "phy", 3))
#elif CONFIG_ETH_PHY_DP83848
    ERROR_CHECK(nvs_set_u32(nvs, "phy", 4))
#endif
#else
    ERROR_CHECK(nvs_set_u8(nvs, "ethernet", false))
    ERROR_CHECK(nvs_set_u32(nvs, "phy", 0))
    ERROR_CHECK(nvs_set_u32(nvs, "phy_addr", 0))
    ERROR_CHECK(nvs_set_u32(nvs, "phy_rst", 0))
    ERROR_CHECK(nvs_set_u32(nvs, "phy_mdc", 0))
    ERROR_CHECK(nvs_set_u32(nvs, "phy_mdio", 0))
#endif

#ifdef CONFIG_WIFI_ENABLE
    ERROR_CHECK(nvs_set_u8(nvs, "wifi", true))
#else
    ERROR_CHECK(nvs_set_u8(nvs, "wifi", false))
#endif

    // Initialize network info to 0
    esp_netif_ip_info_t info = {0};
    ERROR_CHECK(set_network_info(info))

    return ESP_OK;
}

static esp_err_t init_gpio()
{
    ESP_LOGI(TAG, "Saving GPIO Configuration");
#ifdef CONFIG_GPIO_ENABLE
    ERROR_CHECK(nvs_set_i8(nvs, "gpio", true))
    ERROR_CHECK(nvs_set_i32(nvs, "button", CONFIG_BUTTON))
    ERROR_CHECK(nvs_set_i32(nvs, "red_led", CONFIG_RED_LED))
    ERROR_CHECK(nvs_set_i32(nvs, "green_led", CONFIG_GREEN_LED))
    ERROR_CHECK(nvs_set_i32(nvs, "blue_led", CONFIG_BLUE_LED))
#else
    ERROR_CHECK(nvs_set_i8(nvs, "gpio", false))
    ERROR_CHECK(nvs_set_i32(nvs, "button", -1))
    ERROR_CHECK(nvs_set_i32(nvs, "red_led", -1))
    ERROR_CHECK(nvs_set_i32(nvs, "green_led", -1))
    ERROR_CHECK(nvs_set_i32(nvs, "blue_led", -1))
#endif

    return ESP_OK;
}

static esp_err_t first_power_on()
{
    ESP_LOGI(TAG, "First power up");
    ERROR_CHECK(init_settings())
    ERROR_CHECK(init_interfaces())
    ERROR_CHECK(init_gpio())
    ERROR_CHECK(create_log_file())
    ERROR_CHECK(store_default_blacklists())

    ERROR_CHECK(nvs_set_u8(nvs, "initialized", (uint8_t)true))
    
    return ESP_OK;
}

esp_err_t reset_device()
{
    ESP_LOGI(TAG, "Resetting Device");
    ERROR_CHECK(nvs_set_u8(nvs, "initialized", (uint8_t)false))
    return ESP_OK;
}

static esp_err_t init_nvs()
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
        
    ESP_LOGD(TAG, "NVS Initialized");
    return ESP_OK;
}

static esp_err_t init_spiffs()
{
    ESP_LOGI(TAG, "Initializing SPIFFS");
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 10,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Spiffs Partition size: total: %d, used: %d", total, used);
    return ESP_OK;
}

esp_err_t initialize_flash()
{    
    ERROR_CHECK(init_nvs())
    ERROR_CHECK(init_spiffs())


    bool initialized = false;
    if( nvs_get_u8(nvs, "initialized", (uint8_t*)&initialized) == ESP_ERR_NVS_NOT_FOUND || initialized == false)
    {
        ERROR_CHECK(first_power_on())
    }

    ESP_LOGI(TAG, "Flash Initialized");
    return ESP_OK;
}