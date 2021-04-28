#include "post_handlers.h"
#include "webserver.h"
#include "error.h"
#include "events.h"
#include "wifi.h"
#include "filesystem.h"
#include "flash.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char* TAG = "HTTP";


esp_err_t submitauth_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST to /submitauth");

    // Determine if content is too large/small
    if (req->content_len > 128 || req->content_len <= 1) // data shouldn't be longer than 128 bytes
        SEND_ERR(req, HTTPD_400_BAD_REQUEST, "SSID and password are too long")

    // receive data
    char data[req->content_len];
    httpd_req_recv(req, data, req->content_len);

    // parse json
    cJSON* json = cJSON_Parse(data);
    if (json == NULL)
        SEND_ERR(req, HTTPD_400_BAD_REQUEST, "Invalid JSON")

    // Get ssid & pass data
    cJSON* ssid = cJSON_GetObjectItem(json, "ssid");
    cJSON* pass = cJSON_GetObjectItem(json, "pass");
    if ( !cJSON_IsString(ssid) || ssid->valuestring == NULL ||
         !cJSON_IsString(pass) || pass->valuestring == NULL)
    {
        cJSON_Delete(json);
        SEND_ERR(req, HTTPD_400_BAD_REQUEST, "Error reading ssid & password")
    }

    // attempt to connect
    bool connected = false;
    if( attempt_to_connect(ssid->valuestring, pass->valuestring, &connected) != ESP_OK )
    {
        cJSON_Delete(json);
        SEND_ERR(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error while attempting to connect")
    }

    // check result of attempt
    if(connected)
    {
        ESP_LOGI(TAG, "Connection Successful");
        httpd_resp_set_status(req, HTTPD_200);
        httpd_resp_sendstr(req, "connected");
    } 
    else 
    {
        ESP_LOGI(TAG, "Could not connect to wifi");
        httpd_resp_set_status(req, HTTPD_200);
        httpd_resp_sendstr(req, "could not connect");
    }
    
    cJSON_Delete(json);
    return ESP_OK;
}

static httpd_uri_t submitauth = {
    .uri       = "/submitauth",
    .method    = HTTP_POST,
    .handler   = submitauth_handler,
};


esp_err_t scan_handler(httpd_req_t *req)
{
    if( wifi_scan() != ESP_OK )
    {
        SEND_ERR(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error starting scan")
    }

    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_sendstr(req, "could not connect");
    return ESP_OK;
}

static httpd_uri_t scan = {
    .uri       = "/scan",
    .method    = HTTP_POST,
    .handler   = scan_handler,
};


esp_err_t finish_handler(httpd_req_t *req)
{
    // network info will not be zero if last connection attempt was successful
    esp_netif_ip_info_t ip_info;
    get_network_info(&ip_info);

    if( ip_info.ip.addr )
    {
        set_provisioning_status(true);
        httpd_resp_set_status(req, HTTPD_200);
        httpd_resp_sendstr(req, "");
        xTimerHandle restartTimer = xTimerCreate("restart", pdMS_TO_TICKS(500), pdTRUE, (void*)0, (void *)esp_restart);
        xTimerStart(restartTimer, 0);
    }
    else
    {
        SEND_ERR(req, HTTPD_400_BAD_REQUEST, "Previous connection attempt was unsuccessful")
    }

    return ESP_OK;
}

static httpd_uri_t finish = {
    .uri       = "/finish",
    .method    = HTTP_POST,
    .handler   = finish_handler,
};


esp_err_t register_post_handlers(httpd_handle_t server)
{
    ATTEMPT(httpd_register_uri_handler(server, &submitauth))
    ATTEMPT(httpd_register_uri_handler(server, &scan))
    ATTEMPT(httpd_register_uri_handler(server, &finish))

    return ESP_OK;
}