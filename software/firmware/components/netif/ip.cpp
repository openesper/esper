#include "ip.h"
#include "events.h"
#include "error.h"
#include "settings.h"
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
static esp_netif_t* eth_netif = NULL;
static esp_eth_handle_t eth_handle = NULL;


static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    switch (event_id) {
        case IP_EVENT_ETH_GOT_IP:
        {
            ESP_LOGI(TAG, "IP_EVENT_ETH_GOT_IP");

            ip_event_got_ip_t ip_event = *(ip_event_got_ip_t*)event_data;
            sett::write(sett::IP, inet_ntoa(ip_event.ip_info.ip));
            sett::write(sett::NETMASK, inet_ntoa(ip_event.ip_info.netmask));
            sett::write(sett::GATEWAY, inet_ntoa(ip_event.ip_info.gw));
            set_bit(ETH_CONNECTED_BIT);

            break;
        }
        case IP_EVENT_STA_GOT_IP:
        {
            ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");

            ip_event_got_ip_t ip_event = *(ip_event_got_ip_t*)event_data;
            sett::write(sett::IP, inet_ntoa(ip_event.ip_info.ip));
            sett::write(sett::NETMASK, inet_ntoa(ip_event.ip_info.netmask));
            sett::write(sett::GATEWAY, inet_ntoa(ip_event.ip_info.gw));
            set_bit(WIFI_CONNECTED_BIT);

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
    esp_wifi_set_mode(WIFI_MODE_STA);
    ATTEMPT(init_wifi_sta_netif(&wifi_sta_netif))
    
    ESP_LOGI(TAG, "Wifi initialized");

    return ESP_OK;
}

esp_err_t init_interfaces()
{
    // Turn on available interfaces
    if( check_bit(ETH_ENABLED_BIT) )
    {
        ATTEMPT(init_eth())
        set_bit(ETH_INITIALIZED_BIT);
    }
    else if( check_bit(WIFI_ENABLED_BIT) )
    {
        ATTEMPT(init_wifi())
        set_bit(WIFI_INITIALIZED_BIT);
    }

    if( !check_bit(ETH_INITIALIZED_BIT) && !check_bit(WIFI_INITIALIZED_BIT))
    {
        return IP_ERR_INIT;
    }

    return ESP_OK;
}

esp_err_t set_static_ip(esp_netif_t* interface)
{
    // Get network info from flash, if it exists
    esp_netif_ip_info_t ip_info;
    
    std::string ip = sett::read_str(sett::IP);
    if( inet_aton(ip.c_str(), &(ip_info.ip)) )
    {
        ip = sett::read_str(sett::NETMASK);
        ip_info.netmask.addr = inet_addr(ip.c_str());

        ip = sett::read_str(sett::GATEWAY);
        ip_info.gw.addr = inet_addr(ip.c_str());

        esp_netif_dhcpc_stop(interface);
        esp_netif_set_ip_info(interface, &ip_info);
        ESP_LOGI(TAG, "Assigned static IP to interface");
    }
    else
    {
        ESP_LOGW(TAG, "No static IP found");
    }

    esp_netif_dns_info_t dns = {};
    ip = sett::read_str(sett::DNS_SRV);
    inet_aton(ip.c_str(), &dns.ip.u_addr.ip4);
    dns.ip.type = IPADDR_TYPE_V4;
    
    ATTEMPT(esp_netif_set_dns_info(interface, ESP_NETIF_DNS_MAIN, &dns))

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
        esp_wifi_start();
        ATTEMPT(set_static_ip(wifi_sta_netif))
        esp_wifi_connect();
    }

    return ESP_OK;
}

