#include "settings.h"
#include "error.h"
#include "events.h"
#include "dns.h"
#include "ota.h"
#include "flash.h"
#include "url.h"
#include "cJSON.h"
#include "esp_ota_ops.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"


#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "HTTP";

/**
  * @brief Handler restart request, restarts the device 500ms after request
  */
static esp_err_t restart_post_handler(httpd_req_t *req)
{
    // Set timer to restart in 500ms, so response can be sent out
    xTimerHandle restartTimer = xTimerCreate("restart", pdMS_TO_TICKS(500), pdTRUE, (void*)0, (void *)esp_restart);
    xTimerStart(restartTimer, 0);

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_sendstr(req, "");

    return ESP_OK;
}

static httpd_uri_t restart_post = {
        .uri       = "/restart",
        .method    = HTTP_POST,
        .handler   = restart_post_handler,
        .user_ctx  = ""
};

/**
  * @brief Handler for settings GET requests
  */
static esp_err_t settings_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for %s", req->uri);
    long start = esp_timer_get_time();

    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    extern const unsigned char  settings_html_start[] asm("_binary_settings_html_start");
    extern const unsigned char  settings_html_end[]   asm("_binary_settings_html_end");
    const size_t  settings_html_size = (settings_html_end -  settings_html_start);
    httpd_resp_send(req, (const char *)settings_html_start,  settings_html_size);
    ESP_LOGD(TAG, "Settings.html processing Time: %lld ms", (esp_timer_get_time()-start)/1000);

    return ESP_OK;
}

static httpd_uri_t settings_get = {
        .uri       = "/settings",
        .method    = HTTP_GET,
        .handler   = settings_get_handler,
        .user_ctx  = ""
};

/**
  * @brief Handler for settings POST requests
  */
static esp_err_t settings_json_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for %s", req->uri);  
    cJSON *json = cJSON_CreateObject();
    if ( json == NULL )
    {
        cJSON_Delete(json);
        ESP_LOGW(TAG, "Error creating json object");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error creating JSON object");
        return ESP_OK;
    }

    // Get settings from flash and add them to json

    bool blocking = check_bit(BLOCKING_BIT);
    cJSON_AddBoolToObject(json, "blocking", blocking);

    esp_netif_ip_info_t info;
    get_network_info(&info);
    char* ip = inet_ntoa(info.ip);
    cJSON_AddStringToObject(json, "ip", ip);

    char url[MAX_URL_LENGTH];
    get_device_url(url);
    cJSON_AddStringToObject(json, "url", url);

    char dnssrv[IP4ADDR_STRLEN_MAX];
    get_upstream_dns(dnssrv);
    cJSON_AddStringToObject(json, "dnssrv", dnssrv);

    char updatesrv[MAX_URL_LENGTH];
    get_update_url(updatesrv);
    cJSON_AddStringToObject(json, "updatesrv", updatesrv);
    
    const esp_app_desc_t* firmware = esp_ota_get_app_description();
    cJSON_AddStringToObject(json, "version", firmware->version);

    cJSON_AddBoolToObject(json, "update", check_bit(UPDATE_AVAILABLE_BIT));

    char* update_status = get_update_check_status_string(); 
    if( strlen(update_status) )
    {
        cJSON_AddStringToObject(json, "update_status", update_status);
    }

    httpd_resp_set_type(req, "application/json;");
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_sendstr(req, cJSON_Print(json));
    cJSON_Delete(json);

    return ESP_OK;
}

static httpd_uri_t settings_json_get = {
    .uri       = "/settings.json",
    .method    = HTTP_GET,
    .handler   = settings_json_get_handler,
    .user_ctx  = ""
};

/**
  * @brief Handler for settings POST requests
  */
static esp_err_t settings_json_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for %s", req->uri);

    // Verify content is not too long
    if ( req->content_len > (MAX_URL_LENGTH*2+CONFIG_HTTPD_MAX_URI_LEN+IP4ADDR_STRLEN_MAX*2+100) )
    {
        ESP_LOGW(TAG, "Could not add, URL too long");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Content length too large");
        return ESP_OK;
    }

    // check for valid json
    char data[req->content_len];
    httpd_req_recv(req, data, req->content_len);
    cJSON* json = cJSON_Parse(data);
    if (json == NULL)
    {
        ESP_LOGW(TAG, "Invalid json");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid json");
        cJSON_Delete(json);
        return ESP_OK;
    }

    // Get IP
    cJSON* ip = cJSON_GetObjectItem(json, "ip");
    if ( !cJSON_IsString(ip) || ip->valuestring == NULL)
    {
        ESP_LOGW(TAG, "Invalid IP");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid IP");
        cJSON_Delete(json);
        return ESP_OK;
    }
    ESP_LOGI(TAG, "IP: %s", ip->valuestring);

    // Save IP to flash
    esp_netif_ip_info_t info;
    get_network_info(&info);
    inet_aton(ip->valuestring, &(info.ip));
    set_network_info(info);

    // Get URL
    cJSON* url = cJSON_GetObjectItem(json, "url");
    if ( !cJSON_IsString(url) || url->valuestring == NULL)
    {
        ESP_LOGW(TAG, "Invalid url");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid url");
        cJSON_Delete(json);
        return ESP_OK;
    }
    ESP_LOGI(TAG, "url: %s", url->valuestring);

    // Save URL to flash, load updated URL into RAM
    set_device_url(url->valuestring);
    load_device_url();

    // Get dnssrv
    cJSON* dnssrv = cJSON_GetObjectItem(json, "dnssrv");
    if ( !cJSON_IsString(dnssrv) || dnssrv->valuestring == NULL)
    {
        ESP_LOGW(TAG, "Invalid dnssrv");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid dnssrv");
        cJSON_Delete(json);
        return ESP_OK;
    }
    ESP_LOGI(TAG, "dnssrv: %s", dnssrv->valuestring);

    // Save DNS server to flash, load updated DNS server into RAM
    set_upstream_dns(dnssrv->valuestring);
    load_upstream_dns();

    // Get updatesrv
    cJSON* updatesrv = cJSON_GetObjectItem(json, "updatesrv");
    if ( !cJSON_IsString(updatesrv) || updatesrv->valuestring == NULL)
    {
        ESP_LOGW(TAG, "Invalid updatesrv");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid updatesrv");
        cJSON_Delete(json);
        return ESP_OK;
    }
    ESP_LOGI(TAG, "updatesrv: %s", updatesrv->valuestring);

    // Save update url to flash, check for update from new url
    set_update_url(updatesrv->valuestring);
    check_for_update();

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_sendstr(req, "");
    cJSON_Delete(json);

    return ESP_OK;
}

static httpd_uri_t settings_json_post = {
    .uri       = "/settings.json",
    .method    = HTTP_POST,
    .handler   = settings_json_post_handler,
    .user_ctx  = ""
};

/**
  * @brief Handler for toggleblock POST requests
  */
static esp_err_t toggle_blocking_handler(httpd_req_t *req){
    ESP_LOGI(TAG, "Request for %s", req->uri);

    // Toggle blocking bit
    if ( toggle_bit(BLOCKING_BIT) != ESP_OK )
    {
        ESP_LOGW(TAG, "Error toggling blocking status");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error toggling blocking status");
        return ESP_OK;
    }

    // Create JSON response with current blocking status
    cJSON *json = cJSON_CreateObject();
    if ( json == NULL )
    {
        cJSON_Delete(json);
        ESP_LOGW(TAG, "Error creating json object");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error creating JSON object");
        return ESP_OK;
    }

    bool blocking = check_bit(BLOCKING_BIT);
    cJSON_AddBoolToObject(json, "blocking", blocking);

    httpd_resp_set_type(req, "application/json;");
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_sendstr(req, cJSON_Print(json));
    cJSON_Delete(json);

    return ESP_OK;
}

static httpd_uri_t set_blocking_status = {
    .uri       = "/toggleblock",
    .method    = HTTP_POST,
    .handler   = toggle_blocking_handler,
    .user_ctx  = ""
};

/**
  * @brief Handler for OTA POST requests, starts OTA process
  */
static esp_err_t updatefirmware_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for %s", req->uri);

    start_ota();

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_sendstr(req, "");

    return ESP_OK;
}

static httpd_uri_t update_firmware = {
    .uri       = "/updatefirmware",
    .method    = HTTP_POST,
    .handler   = updatefirmware_handler,
    .user_ctx  = ""
};

/**
  * @brief Handler for OTA status GET requests
  */
static esp_err_t ota_status_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for %s", req->uri);

    cJSON *json = cJSON_CreateObject();
    if ( json == NULL )
    {
        cJSON_Delete(json);
        ESP_LOGW(TAG, "Error creating json object");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error creating JSON object");
        return ESP_OK;
    }

    eTaskState ota_state = get_ota_task_status();
    ESP_LOGI(TAG, "OTA status (%d)", ota_state);
    if( ota_state == eRunning || ota_state == eBlocked || ota_state == eReady)
    {
        cJSON_AddBoolToObject(json, "running", true);
    }
    else if(  ota_state == eSuspended )
    {
        cJSON_AddBoolToObject(json, "running", false);

        char* update_status = get_ota_status_string(); 
        if( strlen(update_status) )
        {
            cJSON_AddStringToObject(json, "update_status", update_status);
        }
    }

    httpd_resp_set_type(req, "application/json;");
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_sendstr(req, cJSON_Print(json));
    cJSON_Delete(json);

    return ESP_OK;
}

static httpd_uri_t ota_status = {
    .uri       = "/updatestatus",
    .method    = HTTP_GET,
    .handler   = ota_status_get_handler,
    .user_ctx  = ""
};

/**
  * @brief Setup all handlers needed for settings page
  */
esp_err_t setup_settings_handlers(httpd_handle_t server)
{
    ATTEMPT(httpd_register_uri_handler(server, &restart_post))
    ATTEMPT(httpd_register_uri_handler(server, &settings_get))
    ATTEMPT(httpd_register_uri_handler(server, &settings_json_get))
    ATTEMPT(httpd_register_uri_handler(server, &settings_json_post))
    ATTEMPT(httpd_register_uri_handler(server, &set_blocking_status))
    ATTEMPT(httpd_register_uri_handler(server, &update_firmware))
    ATTEMPT(httpd_register_uri_handler(server, &ota_status))

    return ESP_OK;
}

/**
  * @brief Teardown all settings page handlers
  */
esp_err_t teardown_settings_handlers(httpd_handle_t server)
{
    ATTEMPT(httpd_unregister_uri_handler(server, restart_post.uri, restart_post.method))
    ATTEMPT(httpd_unregister_uri_handler(server, settings_get.uri, settings_get.method))
    ATTEMPT(httpd_unregister_uri_handler(server, settings_json_get.uri, settings_json_get.method))
    ATTEMPT(httpd_unregister_uri_handler(server, settings_json_post.uri, settings_json_post.method))
    ATTEMPT(httpd_unregister_uri_handler(server, set_blocking_status.uri, set_blocking_status.method))
    ATTEMPT(httpd_unregister_uri_handler(server, update_firmware.uri, update_firmware.method))
    ATTEMPT(httpd_unregister_uri_handler(server, ota_status.uri, ota_status.method))

    return ESP_OK;
}