#include "eth.h"
#include "error.h"

#include "esp_system.h"
#include "esp_eth.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
#include "esp_log.h"
static const char *TAG = "ETH";

enum Phy {
    NONE,
    LAN8720,
    IP101,
    RTL8201,
    DP83848
};

static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id) {
        case ETHERNET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "ETHERNET_EVENT_CONNECTED");
            break;
        case ETHERNET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "ETHERNET_EVENT_DISCONNECTED");
            break;
        case ETHERNET_EVENT_START:
            ESP_LOGI(TAG, "ETHERNET_EVENT_START");
            break;
        case ETHERNET_EVENT_STOP:
            ESP_LOGI(TAG, " ETHERNET_EVENT_STOP");
            break;
        default:
            break;
    }
}

esp_netif_t* init_eth_netif()
{
    TRY(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL))

    esp_netif_config_t netif_cfg =     {
        .base = ESP_NETIF_BASE_DEFAULT_ETH,      
        .driver = NULL,                          
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH, 
    };

    esp_netif_t* eth_netif = esp_netif_new(&netif_cfg);
    if( eth_netif == NULL)
    {
        THROWE(ETH_ERR_INIT, "Failed to create eth_netif")
    }

    TRY(esp_eth_set_default_handlers(eth_netif))

     return eth_netif;
}

esp_eth_handle_t init_eth_handle()
{
    // Get hardware configuration from flash
    nvs_handle nvs;
    TRY( nvs_open("storage", NVS_READONLY, &nvs) )

    uint8_t phy_id = 0, addr = 0, rst = 0;
    if( nvs_get_u32(nvs, "phy", (uint32_t*)&phy_id) == ESP_ERR_NVS_NOT_FOUND ) {
        TRY(nvs_get_u8(nvs, "phy", &phy_id))
    }else{
        nvs_erase_key(nvs, "phy");
        nvs_set_u8(nvs, "phy", phy_id);
    }
    if( nvs_get_u32(nvs, "phy_addr", (uint32_t*)&addr) == ESP_ERR_NVS_NOT_FOUND ){
        TRY(nvs_get_u8(nvs, "phy_addr", &addr))
    }else{
        nvs_erase_key(nvs, "phy_addr");
        nvs_set_u8(nvs, "phy_addr", addr);
    }
    if( nvs_get_u32(nvs, "phy_rst", (uint32_t*)&rst) == ESP_ERR_NVS_NOT_FOUND ){
        TRY(nvs_get_u8(nvs, "phy_rst", &rst))
    }else{
        nvs_erase_key(nvs, "phy_rst");
        nvs_set_u8(nvs, "phy_rst", rst);
    }
    
    // Setup phy configuration
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = addr;
    phy_config.reset_gpio_num = rst;

    esp_eth_phy_t* phy;
    switch(phy_id) {
        case LAN8720:
            phy = esp_eth_phy_new_lan8720(&phy_config);
            break;
        case IP101:
            phy = esp_eth_phy_new_ip101(&phy_config);
            break;
        case RTL8201:
            phy = esp_eth_phy_new_rtl8201(&phy_config);
            break;
        case DP83848:
            phy = esp_eth_phy_new_dp83848(&phy_config);
            break;
        default:
            phy = NULL;
            break;
    }
    if( phy == NULL )
    {
        THROWE(ETH_ERR_INIT, "Failed to init PHY")
    }

    // Setup MAC configuration
    uint8_t  mdc = 0, mdio = 0;
    if( nvs_get_u32(nvs, "phy_mdc", (uint32_t*)&mdc) == ESP_ERR_NVS_NOT_FOUND ){
        TRY(nvs_get_u8(nvs, "phy_mdc", &mdc))
    }else{
        nvs_erase_key(nvs, "phy_mdc");
        nvs_set_u8(nvs, "phy_mdc", mdc);
    }
    if( nvs_get_u32(nvs, "phy_mdio", (uint32_t*)&mdio) == ESP_ERR_NVS_NOT_FOUND ){
        TRY(nvs_get_u8(nvs, "phy_mdio", &mdio))
    }else{
        nvs_erase_key(nvs, "phy_mdio");
        nvs_set_u8(nvs, "phy_mdio", mdio);
    }

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    mac_config.smi_mdc_gpio_num = mdc;
    mac_config.smi_mdio_gpio_num = mdio;

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);
    if( mac == NULL )
    {
        THROWE(ETH_ERR_INIT, "Failed to init MAC")
    }

    esp_eth_handle_t eth_handle;
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    TRY( esp_eth_driver_install(&config, &eth_handle) )

    return eth_handle;
}