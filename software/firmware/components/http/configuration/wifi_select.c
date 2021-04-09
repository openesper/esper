#include "wifi_select.h"
// #include "accesspoint.h"
#include "error.h"
#include "wifi.h"
#include "esp_wifi.h"
#include <esp_http_server.h>
#include "cJSON.h"
#include "ctype.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "HTTP";

/**
  * @brief Page that lists available SSIDs
  */
static esp_err_t wifi_select_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for /");
    httpd_resp_set_type(req, "text/html; charset=UTF-8");

    extern const unsigned char  wifi_select_html_start[] asm("_binary_wifi_select_html_start");
    extern const unsigned char  wifi_select_html_end[]   asm("_binary_wifi_select_html_end");
    const size_t  wifi_select_html_size = (wifi_select_html_end -  wifi_select_html_start);
    httpd_resp_send_chunk(req, (const char *)wifi_select_html_start, wifi_select_html_size);
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}

static httpd_uri_t wifi_select = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = wifi_select_get_handler,
    .user_ctx  = ""
};

/**
  * @brief Get available IPs
  */
static esp_err_t wifi_json_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for wifi.json");
    httpd_resp_set_type(req, "application/json");

    // Get query
    size_t query_len = httpd_req_get_url_query_len(req) + 1;
    if ( query_len > 16 )
    {
        ESP_LOGI(TAG, "Query too large (%d)", query_len);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Query too large");
        return ESP_OK;
    }

    // If no query exists, default to rescan=false
    if (query_len > 1)
    {
        // Get query string
        char query[query_len + 1];
        if ( httpd_req_get_url_query_str(req, query, query_len) != ESP_OK)
        {
            ESP_LOGI(TAG, "Invalid query");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid query");
            return ESP_OK;
        }

        // Get rescan parameter
        char param[6];
        if ( httpd_query_key_value(query, "rescan", param, sizeof(param)) != ESP_OK ) 
        {
            ESP_LOGI(TAG, "Invalid parameter");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid parameter");
            return ESP_OK;
        }
        
        // determine is parameter == true
        ESP_LOGD(TAG, "Rescan: %s", param);
        if (strcmp("true", param) == 0)
        {
            ESP_LOGI(TAG, "Starting scan");
            wifi_scan();
        }
    }

    // Send list of available SSIDs
    cJSON* wifi = cJSON_CreateObject(); 
    cJSON* ssid_array = cJSON_CreateArray();
    cJSON_AddItemToObject(wifi, "ssid_array", ssid_array);

    wifi_ap_record_t* list = scan_results();
    for(int i = 0; i < MAX_SCAN_RECORDS; i++)
    {
        cJSON* ssid = cJSON_CreateString((char*)list[i].ssid);
        cJSON_AddItemToArray(ssid_array, ssid);
    }    

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_sendstr(req, cJSON_Print(wifi));
    cJSON_Delete(wifi);
    return ESP_OK;
}

static httpd_uri_t wifi_json = {
    .uri       = "/wifi.json",
    .method    = HTTP_GET,
    .handler   = wifi_json_get_handler,
    .user_ctx  = ""
};

static esp_err_t wifi_auth_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST to /submitauth");

    // Determine if content is too large/small
    if (req->content_len > 128 || req->content_len <= 1) // data shouldn't be longer than 128 bytes
    {
        ESP_LOGW(TAG, "Auth data too large");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid data");
        return ESP_OK;
    }

    // Copy data, verify it's valid JSON
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

    // Get ssid & pass data
    cJSON* ssid = cJSON_GetObjectItem(json, "ssid");
    cJSON* pass = cJSON_GetObjectItem(json, "pass");
    if ( !cJSON_IsString(ssid) || ssid->valuestring == NULL ||
         !cJSON_IsString(pass) || pass->valuestring == NULL)
    {
        ESP_LOGW(TAG, "Invalid key/value");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid key/value");
        cJSON_Delete(json);
        return ESP_OK;
    }

    // Copy ssid & pass to wifi_config
    wifi_config_t wifi_config = {0};
    strcpy((char*)wifi_config.sta.ssid, ssid->valuestring);
    strcpy((char*)wifi_config.sta.password, pass->valuestring);
    ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    cJSON_Delete(json);

    ESP_LOGI(TAG, "SSID: %s (%d)", (char*)&wifi_config.sta.ssid, strlen((char*)&wifi_config.sta.ssid));
    ESP_LOGI(TAG, "PASS: %s (%d)", (char*)&wifi_config.sta.password, strlen((char*)&wifi_config.sta.password));

    // Attempt to connect to network
    bool connected = false;
    ERROR_CHECK(attempt_to_connect(&connected))
    if(connected)
    {
        ESP_LOGI(TAG, "Connection Successful");
        httpd_resp_set_status(req, "200 OK");
        httpd_resp_sendstr(req, "");
    } 
    else 
    {
        ESP_LOGI(TAG, "Could not connect to wifi");
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_sendstr(req, "");
    }
    
    return ESP_OK;
}

static httpd_uri_t submit = {
    .uri       = "/submitauth",
    .method    = HTTP_POST,
    .handler   = wifi_auth_handler,
    .user_ctx  = ""
};

esp_err_t configure_wifi_select_handler(httpd_handle_t server)
{
    ERROR_CHECK(httpd_register_uri_handler(server, &wifi_select))
    ERROR_CHECK(httpd_register_uri_handler(server, &wifi_json))
    ERROR_CHECK(httpd_register_uri_handler(server, &submit))

    return ESP_OK;
}

esp_err_t teardown_wifi_select_handler(httpd_handle_t server)
{
    ERROR_CHECK(httpd_unregister_uri_handler(server, wifi_select.uri, wifi_select.method))
    ERROR_CHECK(httpd_unregister_uri_handler(server, wifi_json.uri, wifi_json.method))
    ERROR_CHECK(httpd_unregister_uri_handler(server, submit.uri, submit.method))

    return ESP_OK;
}