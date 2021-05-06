#include "ip.h"
#include "events.h"
#include "error.h"
#include "settings.h"
#include "eth.h"
#include "wifi.h"
#include "string.h"
// 
#include "filesystem.h"
#include "lwip/sockets.h"

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
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

void init_interfaces()
{
    // Initialize TCP/IP stack
    TRY(esp_netif_init())
    TRY(esp_event_loop_create_default())
    TRY(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL))

    // Turn on available interfaces
    if( check_bit(ETH_ENABLED_BIT) )
    {
        eth_netif = init_eth_netif();
        eth_handle = init_eth_handle();
        TRY(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)))

        set_bit(ETH_INITIALIZED_BIT);
        ESP_LOGI(TAG, "Ethernet initialized");
    }
    else if( check_bit(WIFI_ENABLED_BIT) )
    {
        wifi_sta_netif = init_wifi_sta_netif();
        
        set_bit(WIFI_INITIALIZED_BIT);
        ESP_LOGI(TAG, "Wifi initialized");
    }
    else
    {
        THROWE(IP_ERR_INIT, "No interfaces are enabled")
    }
}

static void set_static_ip(esp_netif_t* interface)
{
    // Get network info from flash, if it exists
    esp_netif_ip_info_t ip_info;
    
    try{
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
            ESP_LOGW(TAG, "No static IP in settings.json");
        }

        esp_netif_dns_info_t dns = {};
        ip = sett::read_str(sett::DNS_SRV);
        inet_aton(ip.c_str(), &dns.ip.u_addr.ip4);
        dns.ip.type = IPADDR_TYPE_V4;
        esp_netif_set_dns_info(interface, ESP_NETIF_DNS_MAIN, &dns);
    }
    catch(const Err& e){
        ESP_LOGE(TAG, "Error setting static ip, turing DHCP on");
        esp_netif_dhcpc_start(interface);

        esp_netif_dns_info_t dns = {};
        inet_aton("8.8.8.8", &dns.ip.u_addr.ip4);
        dns.ip.type = IPADDR_TYPE_V4;
        esp_netif_set_dns_info(interface, ESP_NETIF_DNS_MAIN, &dns);
    }
}

void start_interfaces()
{
    if( check_bit(ETH_INITIALIZED_BIT) )
    {
        ESP_LOGI(TAG, "Starting Ethernet");
        set_static_ip(eth_netif);
        TRY(esp_eth_start(eth_handle))
    }

    if( check_bit(WIFI_INITIALIZED_BIT) )
    {
        esp_wifi_start();
        set_static_ip(wifi_sta_netif);
        esp_wifi_connect();
    }
}

