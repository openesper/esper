#include "wifi.h"
#include "events.h"
#include "ip.h"
#include "error.h"
#include "filesystem.h"
#include "flash.h"
#include "string.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "cJSON.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "WIFI";

static esp_err_t store_ap_records()
{
    uint16_t number = MAX_SCAN_RECORDS;
    static wifi_ap_record_t ap_list[MAX_SCAN_RECORDS];
    esp_wifi_scan_get_ap_records(&number, ap_list);

    FILE* wifi_json = open_file("/prov/wifi.json", "w");
    if( !wifi_json )
    {
        log_error(ESP_FAIL, "open_file(\"/prov/wifi.json\", \"w\")", __func__, __FILE__);
        return ESP_FAIL;
    }

    cJSON* json = cJSON_CreateArray();
    if( !json )
    {
        log_error(ESP_FAIL, "cJSON_CreateArray()", __func__, __FILE__);
        fclose(wifi_json);
        return ESP_FAIL;
    }

    for( int i = 0; i < number; i++)
    {
        cJSON* ap = cJSON_CreateObject();
        if( ap )
        {
            cJSON_AddStringToObject(ap, "ssid", (char*)ap_list[i].ssid);
            cJSON_AddNumberToObject(ap, "rssi", (double)ap_list[i].rssi);
            cJSON_AddNumberToObject(ap, "authmode", (double)ap_list[i].authmode);
            cJSON_AddItemToArray(json, ap);
        }
    }

    char* json_str = cJSON_Print(json);
    if( fwrite(json_str, 1, strlen(json_str), wifi_json) < strlen(json_str) )
    {
        log_error(ESP_FAIL, "fwrite(json_str, 1, strlen(json_str), wifi_json)", __func__, __FILE__);
    }

    cJSON_Delete(json);
    fclose(wifi_json);
    ESP_LOGI(TAG, "Saved scan results to /prov/wifi.json");
    return ESP_OK;
}

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
            store_ap_records();
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
            // set_bit(WIFI_CONNECTED_BIT);
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED:
        {
            wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
            ESP_LOGW(TAG, "WIFI_EVENT_STA_DISCONNECTED %d", event->reason);
            clear_bit(WIFI_CONNECTED_BIT);
            break;
        }
        case WIFI_EVENT_STA_AUTHMODE_CHANGE:
        {
            ESP_LOGI(TAG, "WIFI_EVENT_STA_AUTHMODE_CHANGE");
            // (wifi_event_sta_authmode_change_t*)event_data;
            break;
        }
        case WIFI_EVENT_AP_START:
        {
            ESP_LOGI(TAG, "WIFI_EVENT_AP_START");
            esp_wifi_scan_start(NULL, false);
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
            ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            ESP_LOGW(TAG, "WIFI_EVENT_AP_STADISCONNECTED");
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
            ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
            break;
        }
        case WIFI_EVENT_AP_PROBEREQRECVED:
        {
            ESP_LOGI(TAG, "WIFI_EVENT_AP_PROBEREQRECVED");
            // (wifi_event_ap_probe_req_rx_t*)event_data;
            break;
        }
        default:
            break;
    }
}

esp_err_t init_wifi_sta_netif(esp_netif_t** sta_netif)
{
    esp_netif_config_t sta_cfg = {                                                 
        .base = ESP_NETIF_BASE_DEFAULT_WIFI_STA,      
        .driver = NULL,                               
        .stack = ESP_NETIF_NETSTACK_DEFAULT_WIFI_STA, 
    };

    *sta_netif = esp_netif_new(&sta_cfg);
    if( !*sta_netif )
        return ESP_FAIL;
    
    ATTEMPT(esp_netif_attach_wifi_station(*sta_netif))
    ATTEMPT(esp_wifi_set_default_wifi_sta_handlers())

    return ESP_OK;
}

esp_err_t init_wifi_ap_netif(esp_netif_t** ap_netif)
{
    esp_netif_config_t ap_cfg = {                                                 
        .base = ESP_NETIF_BASE_DEFAULT_WIFI_AP,      
        .driver = NULL,                               
        .stack = ESP_NETIF_NETSTACK_DEFAULT_WIFI_AP, 
    };

    *ap_netif = esp_netif_new(&ap_cfg);
    if( !*ap_netif )
        return ESP_FAIL;
    
    ATTEMPT(esp_netif_attach_wifi_ap(*ap_netif))
    ATTEMPT(esp_wifi_set_default_wifi_ap_handlers())

    wifi_config_t ap_config = {
        .ap = {
            .ssid = "esper",
            .ssid_len = 5,
            .channel = 1,
            .password = "",
            .max_connection = 5,
            .authmode = WIFI_AUTH_OPEN
        },
    };
    ATTEMPT(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config))

    return ESP_OK;
}

esp_err_t configure_wifi()
{
    ATTEMPT(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL))

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ATTEMPT(esp_wifi_init(&init_cfg))

    return ESP_OK;
}

esp_err_t wifi_scan()
{
    ESP_LOGI(TAG, "Starting scan...");
    ATTEMPT(esp_wifi_scan_start(NULL, true))
    return ESP_OK;
}

esp_err_t attempt_to_connect(char* ssid, char* pass, bool* result)
{
    ESP_LOGI(TAG, "Attempting to connect to AP");

    if( !result || !ssid || !pass )
        return ESP_ERR_INVALID_ARG;

    // Disconnect if already connected to AP
    esp_wifi_disconnect();

    // clear connection.json
    FILE* f = open_file("/prov/connection.json", "w");
    fclose(f);

    // clear saved ip info
    esp_netif_ip_info_t ip_info = {0};
    set_network_info(ip_info);

    // Copy ssid & pass to wifi_config
    wifi_config_t wifi_config = {0};
    strcpy((char*)wifi_config.sta.ssid, ssid);
    strcpy((char*)wifi_config.sta.password, pass);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);

    // try connecting
    ESP_LOGI(TAG, "SSID: %s (%d)", (char*)&wifi_config.sta.ssid, strlen((char*)&wifi_config.sta.ssid));
    ESP_LOGI(TAG, "PASS: %s (%d)", (char*)&wifi_config.sta.password, strlen((char*)&wifi_config.sta.password));
    ATTEMPT(esp_wifi_connect())

    // wait for result
    wait_for(WIFI_CONNECTED_BIT, 10000 / portTICK_PERIOD_MS);
    *result = check_bit(WIFI_CONNECTED_BIT);

    return ESP_OK;
}