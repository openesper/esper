#include "eth.h"
#include "error.h"
#include "flash.h"
#include "esp_system.h"
#include "esp_eth.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "ETH";

esp_netif_t* eth_netif = NULL;
esp_eth_handle_t eth_handle = NULL;

static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id) {
        case ETHERNET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "ETHERNET_EVENT_CONNECTED");
            // esp_wifi_disconnect();
            break;
        case ETHERNET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "ETHERNET_EVENT_DISCONNECTED");
            // esp_wifi_connect();
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

esp_err_t init_eth_netif(esp_netif_t** eth_netif)
{
    // ATTEMPT(esp_netif_init())
    ATTEMPT(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL))

    esp_netif_config_t netif_cfg =     {
        .base = ESP_NETIF_BASE_DEFAULT_ETH,      
        .driver = NULL,                          
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH, 
    };

    *eth_netif = esp_netif_new(&netif_cfg);
    NULL_CHECK(eth_netif)

    // Set default handlers to process TCP/IP stuffs
     ATTEMPT(esp_eth_set_default_handlers(*eth_netif))

    return ESP_OK;
}

esp_err_t init_eth_handle(esp_eth_handle_t* eth_handle)
{
    // Get hardware configuration from flash
    esp_eth_phy_t* phy;
    uint32_t phy_id = 0, addr = 0, rst = 0, mdc = 0, mdio = 0;
    get_ethernet_phy_config(&phy_id, &addr, &rst, &mdc, &mdio);
    
    // Setup phy configuration
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = addr;
    phy_config.reset_gpio_num = rst;

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
    NULL_CHECK(phy)

    // Setup MAC configuration
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    mac_config.smi_mdc_gpio_num = mdc;
    mac_config.smi_mdio_gpio_num = mdio;

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);
    NULL_CHECK(mac)

    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    ERROR_CHECK(esp_eth_driver_install(&config, eth_handle))

    return ESP_OK;
}

// esp_err_t init_eth()
// {
//     ESP_LOGI(TAG, "Initializing Ethernet...");
//     ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL))

//     return ESP_OK;
// }