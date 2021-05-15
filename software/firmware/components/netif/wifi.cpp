#include "wifi.h"
#include "events.h"
#include "ip.h"
#include "error.h"
#include "filesystem.h"
#include "settings.h"

#include "string.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "lwip/inet.h"

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
#include "esp_log.h"
static const char *TAG = "WIFI";


static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    switch (event_id) {
        case WIFI_EVENT_WIFI_READY:
        {
            ESP_LOGI(TAG, "WIFI_EVENT_WIFI_READY");
            break;
        }
        case WIFI_EVENT_SCAN_DONE:
        {
            ESP_LOGI(TAG, "WIFI_EVENT_SCAN_DONE");
            break;
        }
        case WIFI_EVENT_STA_START:
        {
            ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
            break;
        }
        case WIFI_EVENT_STA_STOP:
        {
            ESP_LOGI(TAG, "WIFI_EVENT_STA_STOP");
            break;
        }
        case WIFI_EVENT_STA_CONNECTED:
        {
            ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
            // (wifi_event_sta_connected_t*)event_data;
            set_bit(WIFI_CONNECTED_BIT);
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED:
        {
            wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
            ESP_LOGW(TAG, "WIFI_EVENT_STA_DISCONNECTED %d", event->reason);
            clear_bit(WIFI_CONNECTED_BIT);
            esp_wifi_connect();
            break;
        }
        case WIFI_EVENT_STA_AUTHMODE_CHANGE:
        {
            ESP_LOGI(TAG, "WIFI_EVENT_STA_AUTHMODE_CHANGE");
            break;
        }
        case WIFI_EVENT_AP_START:
        {
            ESP_LOGI(TAG, "WIFI_EVENT_AP_START");
            break;
        }
        case WIFI_EVENT_AP_STOP:
        {
            ESP_LOGI(TAG, "WIFI_EVENT_AP_STOP");
            break;
        }
        case WIFI_EVENT_AP_STACONNECTED:
        {
            ESP_LOGI(TAG, "WIFI_EVENT_AP_STACONNECTED");
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            ESP_LOGW(TAG, "WIFI_EVENT_AP_STADISCONNECTED");
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
            ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
            break;
        }
        case WIFI_EVENT_AP_PROBEREQRECVED:
        {
            ESP_LOGI(TAG, "WIFI_EVENT_AP_PROBEREQRECVED");
            break;
        }
        default:
            break;
    }
}

esp_netif_t* init_wifi_sta_netif()
{
    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    TRY(esp_wifi_init(&init_cfg))
    TRY(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL))

    esp_netif_config_t sta_cfg = {                                                 
        .base = ESP_NETIF_BASE_DEFAULT_WIFI_STA,      
        .driver = NULL,                               
        .stack = ESP_NETIF_NETSTACK_DEFAULT_WIFI_STA, 
    };

    esp_netif_t* sta_netif;
    IF_NULL(sta_netif = esp_netif_new(&sta_cfg))
    {
        THROWE(WIFI_ERR_NULL_NETIF, "Failed to creat instance of sta netif")
    }
    
    TRY(esp_netif_attach_wifi_station(sta_netif))
    TRY(esp_wifi_set_default_wifi_sta_handlers())

    std::string ssid = setting::read_str(setting::SSID);
    std::string pass = setting::read_str(setting::PASSWORD);

    ESP_LOGI(TAG, "SSID: %s", ssid.c_str());
    ESP_LOGI(TAG, "PASS: %s", pass.c_str());
    
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, ssid.c_str());
    strcpy((char*)wifi_config.sta.password, pass.c_str());
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_set_mode(WIFI_MODE_STA);

    return sta_netif;
}