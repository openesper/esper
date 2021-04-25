#include "ip.h"
#include "events.h"
#include "error.h"
#include "eth.h"
#include "wifi.h"
#include "string.h"
#include "flash.h"
#include "filesystem.h"
#include "lwip/sockets.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "IP";

static esp_netif_t* wifi_sta_netif = NULL;
static esp_netif_t* wifi_ap_netif = NULL;
static esp_netif_t* eth_netif = NULL;
static esp_eth_handle_t eth_handle = NULL;


static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    switch (event_id) {
        case IP_EVENT_ETH_GOT_IP:
        {
            ESP_LOGI(TAG, "IP_EVENT_ETH_GOT_IP");

            ip_event_got_ip_t ip_event = *(ip_event_got_ip_t*)event_data;
            set_network_info(ip_event.ip_info);
            set_bit(ETH_CONNECTED_BIT);

            break;
        }
        case IP_EVENT_STA_GOT_IP:
        {
            ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
             ip_event_got_ip_t ip_event = *(ip_event_got_ip_t*)event_data;
            
            if( check_bit(PROVISIONING_BIT) )
            {               
                FILE* connection = open_file("/prov/connection.json", "w");
                if( connection )
                {
                    cJSON* json = cJSON_CreateObject();
                    if( json )
                    {
                        wifi_config_t wifi_config = {0};
                        esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);
                        cJSON_AddStringToObject(json, "ssid", (char*)wifi_config.ap.ssid);
                        cJSON_AddStringToObject(json, "ip", inet_ntoa(ip_event.ip_info.ip));

                        char* json_str = cJSON_Print(json);
                        if( fwrite(json_str, 1, strlen(json_str), connection) == strlen(json_str) )
                        {
                            set_network_info(ip_event.ip_info);
                            set_bit(WIFI_CONNECTED_BIT);
                        }
                        else
                        {
                            ESP_LOGE(TAG, "Error saving ssid & password");
                        }
                    }
                    cJSON_Delete(json);
                }
                fclose(connection);
            }
            else{
                set_bit(WIFI_CONNECTED_BIT);
            }

            break;
        }
        case IP_EVENT_STA_LOST_IP:
            ESP_LOGI(TAG, "IP_EVENT_STA_LOST_IP");
            break;
        case IP_EVENT_AP_STAIPASSIGNED:
            ESP_LOGI(TAG, "IP_EVENT_AP_STAIPASSIGNED");
            // New device connected to accesspoint
            break;
        default:
            break;
    }
}

esp_err_t init_tcpip()
{
    // Initialize TCP/IP stack
    ATTEMPT(esp_netif_init())
    ATTEMPT(esp_event_loop_create_default())
    ATTEMPT(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL))

    return ESP_OK;
}

static esp_err_t init_eth()
{
    ATTEMPT(init_eth_netif(&eth_netif))
    ATTEMPT(init_eth_handle(&eth_handle))
    ATTEMPT(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)))
    ESP_LOGI(TAG, "Ethernet initialized");

    return ESP_OK;
}

static esp_err_t init_wifi()
{
    ATTEMPT(configure_wifi())
    ATTEMPT(init_wifi_sta_netif(&wifi_sta_netif))
    ATTEMPT(init_wifi_ap_netif(&wifi_ap_netif))
    ESP_LOGI(TAG, "Wifi initialized");

    return ESP_OK;
}

esp_err_t init_interfaces()
{
    // Turn on available interfaces
    if( check_bit(ETH_ENABLED_BIT) )
    {
        if( init_eth() != ESP_OK )
        {
            log_error(ESP_FAIL, "Failed to init ethernet");
        }
        else
        {
            set_bit(ETH_INITIALIZED_BIT);
        }
    }
    else if( check_bit(WIFI_ENABLED_BIT) )
    {
        if( init_wifi() != ESP_OK )
        {
            log_error(ESP_FAIL, "Failed to init WIFI");
        }
        else
        {
            set_bit(WIFI_INITIALIZED_BIT);
        }
    }

    if( !check_bit(ETH_INITIALIZED_BIT) && !check_bit(WIFI_INITIALIZED_BIT))
    {
        log_error(IP_ERR_INIT, "Failed to initialize interface");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t set_static_ip(esp_netif_t* interface)
{
    // Get network info from flash
    esp_netif_ip_info_t ip_info;
    get_network_info(&ip_info);

    // Static IP may not be assigned yet
    if( ip_info.ip.addr )
    {
        esp_netif_dhcpc_stop(interface);
        esp_netif_set_ip_info(interface, &ip_info);
        ESP_LOGI(TAG, "Assigned static IP to interface");
    }
    else
    {
        ESP_LOGW(TAG, "No static IP found");
    }

    cJSON* json = get_settings_json();
    if( json == NULL)
    {
        log_error(ESP_ERR_NOT_FOUND, "Failed to open settings");
        return ESP_FAIL;
    }

    esp_netif_dns_info_t dns = {0};
    cJSON* upstream_dns = cJSON_GetObjectItem(json, "upstream_server");
    ip4addr_aton(upstream_dns->valuestring, (ip4_addr_t*)&dns.ip.u_addr.ip4);
    dns.ip.type = IPADDR_TYPE_V4;
    
    ATTEMPT(esp_netif_set_dns_info(interface, ESP_NETIF_DNS_MAIN, &dns))

    cJSON_Delete(json);
    return ESP_OK;
}

esp_err_t start_interfaces()
{
    if( check_bit(ETH_INITIALIZED_BIT) )
    {
        ESP_LOGI(TAG, "Starting Ethernet");
        ATTEMPT(set_static_ip(eth_netif))
        ATTEMPT(esp_eth_start(eth_handle))
    }

    if( check_bit(WIFI_INITIALIZED_BIT) )
    {
        if( check_bit(PROVISIONING_BIT) )
        {
            ESP_LOGI(TAG, "Starting Wifi AP");
            esp_wifi_set_mode(WIFI_MODE_APSTA);
            esp_wifi_start();
        }
        else
        {
            ESP_LOGI(TAG, "Starting Wifi Station");
            esp_wifi_set_mode(WIFI_MODE_STA);
            esp_wifi_start();
            esp_wifi_connect();
        }   
        
    }

    return ESP_OK;
}

esp_err_t turn_on_accesspoint()
{
    ESP_LOGI(TAG, "Turning Accesspoint on");
    ATTEMPT(esp_wifi_set_mode(WIFI_MODE_APSTA))
    ATTEMPT(init_wifi_ap_netif(&wifi_ap_netif))

    return ESP_OK;
}

esp_err_t turn_off_accesspoint()
{
    ESP_LOGI(TAG, "Turning Accesspoint off");
    esp_netif_destroy(wifi_ap_netif);
    return ESP_OK;
}

