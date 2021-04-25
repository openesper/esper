#include "connected.h"
#include "error.h"
#include "flash.h"
#include "url.h"
#include "logging.h"
#include "esp_wifi.h"
#include <esp_http_server.h>
#include "cJSON.h"
#include "freertos/timers.h"
#include "esp_netif.h"
#include "lwip/inet.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "HTTP";

/**
  * @brief Response to request for SSID and IP
  */
static esp_err_t connection_json_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for /connection.json");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_status(req, "200 OK");

    cJSON *connection = cJSON_CreateObject(); 
    
    // Get current SSID
    wifi_config_t wifi_config;
    esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);
    cJSON_AddStringToObject(connection, "ssid", (char*)wifi_config.sta.ssid);

    // Get current IP
    esp_netif_ip_info_t info;
    get_network_info(&info);
    char* ip = inet_ntoa(info.ip);
    cJSON_AddStringToObject(connection, "ip", ip);

    httpd_resp_sendstr(req, cJSON_Print(connection));
    return ESP_OK;
}

static httpd_uri_t connection_json = {
    .uri       = "/connection.json",
    .method    = HTTP_GET,
    .handler   = connection_json_get_handler,
    .user_ctx  = ""
};

/**
  * @brief Page shown after successful connection
  */
static esp_err_t connected_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for /connected");

    httpd_resp_set_type(req, "text/html; charset=UTF-8");

    extern const unsigned char  connected_html_start[] asm("_binary_connected_html_start");
    extern const unsigned char  connected_html_end[]   asm("_binary_connected_html_end");
    const size_t  connected_html_size = (connected_html_end -  connected_html_start);
    httpd_resp_send_chunk(req, (const char *)connected_html_start, connected_html_size);
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}

static httpd_uri_t connected = {
    .uri       = "/connected",
    .method    = HTTP_GET,
    .handler   = connected_get_handler,
    .user_ctx  = ""
};

/**
  * @brief Need to wrap esp_restart() to use it with timer
  */
static void restart( TimerHandle_t xTimer )
{
    ESP_LOGW(TAG, "Restarting...");
    esp_restart();
}

/**
  * @brief Provisioning complete
  */
static esp_err_t finish_setup_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Finishing setup");

    ESP_LOGI(TAG, "Configuration finished");
    ATTEMPT(set_provisioning_status(true))
    xTimerHandle restartTimer = xTimerCreate("Restart timer", pdMS_TO_TICKS(1000), pdFALSE, (void*)0, restart);
    xTimerStart(restartTimer, 0);
    
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_sendstr(req, "");
    return ESP_OK;
}

static httpd_uri_t finished = {
    .uri       = "/finish",
    .method    = HTTP_POST,
    .handler   = finish_setup_handler,
    .user_ctx  = ""
};

esp_err_t configure_connected_handler(httpd_handle_t server)
{
    ATTEMPT(httpd_register_uri_handler(server, &connected));
    ATTEMPT(httpd_register_uri_handler(server, &connection_json));
    ATTEMPT(httpd_register_uri_handler(server, &finished));

    return ESP_OK;
}

esp_err_t teardown_connected_handler(httpd_handle_t server)
{
    ATTEMPT(httpd_unregister_uri_handler(server, connected.uri, connected.method));
    ATTEMPT(httpd_unregister_uri_handler(server, connection_json.uri, connection_json.method));
    ATTEMPT(httpd_unregister_uri_handler(server, finished.uri, finished.method));

    return ESP_OK;
}