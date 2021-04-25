#include "blacklist.h"
#include "flash.h"
#include "error.h"
#include "url.h"
#include "cJSON.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "HTTP";

/**
  * @brief Handler blacklist.json GET requests
  */
static esp_err_t blacklist_json_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for %s", req->uri);
    long start = esp_timer_get_time();

    // Build blacklist json
    cJSON* json = cJSON_CreateObject();
    if( build_blacklist_json(json) != ESP_OK)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error building json");
        cJSON_Delete(json);
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_sendstr(req, cJSON_Print(json));
    cJSON_Delete(json);

    ESP_LOGD(TAG, "blacklist.json processing Time: %lld ms", (esp_timer_get_time()-start)/1000); 
    return ESP_OK;
}

static httpd_uri_t blacklist_json = {
    .uri       = "/blacklist.json",
    .method    = HTTP_GET,
    .handler   = blacklist_json_get_handler,
    .user_ctx  = ""
};

static esp_err_t blacklist_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for %s", req->uri);
    long start = esp_timer_get_time();

    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    extern const unsigned char  blacklist_html_start[] asm("_binary_blacklist_html_start");
    extern const unsigned char  blacklist_html_end[]   asm("_binary_blacklist_html_end");
    const size_t  blacklist_html_size = (blacklist_html_end -  blacklist_html_start);
    httpd_resp_send(req, (const char *)blacklist_html_start,  blacklist_html_size);

    ESP_LOGD(TAG, "Blacklist.html rocessing Time: %lld ms", (esp_timer_get_time()-start)/1000);
    return ESP_OK;
}

static httpd_uri_t blacklist = {
    .uri       = "/blacklist",
    .method    = HTTP_GET,
    .handler   = blacklist_get_handler,
    .user_ctx  = ""
};

static esp_err_t blacklist_add_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for %s", req->uri);
    long start = esp_timer_get_time();

    // Make sure URL isn't too long
    if (req->content_len > MAX_URL_LENGTH + 11)
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

    // Get url from json
    cJSON* json_url = cJSON_GetObjectItem(json, "url");
    if ( !cJSON_IsString(json_url) || json_url->valuestring == NULL)
    {
        ESP_LOGW(TAG, "Invalid key/value");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid key/value");
        cJSON_Delete(json);
        return ESP_OK;
    }

    // Create url struct
    URL url = {};
    url.length = strlen(json_url->valuestring);
    memcpy(url.string, json_url->valuestring, url.length);

    // Add to blacklist, return appropriate status
    esp_err_t err = add_to_blacklist(url);
    switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG, "Added %.*s(%d) to blacklist", url.length, url.string, url.length);
            httpd_resp_set_status(req, "200 OK");
            httpd_resp_sendstr(req, "");
            break;

        case URL_ERR_INVALID_URL:
            ESP_LOGW(TAG, "Could not add %.*s(%d), invalid url", url.length, url.string, url.length);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid url");
            break;

        case URL_ERR_LIST_FUll:
            ESP_LOGW(TAG, "Could not add %.*s(%d), not enough memory", url.length, url.string, url.length);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Not enough memory");
            break;

        case URL_ERR_LIST_UNAVAILBLE:
            ESP_LOGW(TAG, "Could not add %.*s(%d), error opening blacklist.json", url.length, url.string, url.length);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not open blacklist file");
            break;

        case URL_ERR_ALREADY_EXISTS:
            ESP_LOGW(TAG, "Could not add %.*s(%d), already in blacklist", url.length, url.string, url.length);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Already in blacklist");
            break;

        default:
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unknown error");
            break;

    }

    cJSON_Delete(json);
    ESP_LOGD(TAG, "Blacklist add processing Time: %lld ms", (esp_timer_get_time()-start)/1000);
    return ESP_OK;
}

static httpd_uri_t blacklist_add = {
    .uri       = "/blacklist/add",
    .method    = HTTP_POST,
    .handler   = blacklist_add_handler,
    .user_ctx  = ""
};

static esp_err_t blacklist_delete_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for %s", req->uri);
    long start = esp_timer_get_time();

    // Make sure URL isn't too long
    if (req->content_len > MAX_URL_LENGTH + 11)
    {
        ESP_LOGW(TAG, "Could not delete, URL too long");
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

    // Get url
    cJSON* json_url = cJSON_GetObjectItem(json, "url");
    if ( !cJSON_IsString(json_url) || json_url->valuestring == NULL)
    {
        ESP_LOGW(TAG, "Invalid key/value");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid key/value");
        cJSON_Delete(json);
        return ESP_OK;
    }

    // Create url struct
    URL url = {};
    url.length = strlen(json_url->valuestring);
    memcpy(url.string, json_url->valuestring, url.length);

    // remove from blacklist, return appropriate code
    esp_err_t err = remove_from_blacklist(url);
    switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG, "%.*s(%d) removed from blacklist", url.length, url.string, url.length);
            httpd_resp_set_status(req, "200 OK");
            httpd_resp_sendstr(req, "");
            break;

        case URL_ERR_NOT_FOUND:
            ESP_LOGW(TAG, "%.*s(%d) not found in blacklist", url.length, url.string, url.length);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Url not in blacklist");
            break;

        case URL_ERR_LIST_UNAVAILBLE:
            ESP_LOGW(TAG, "Could not remove %.*s(%d), error opening blacklist.json", url.length, url.string, url.length);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not open blacklist file");
            break;

        default:
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unknown error");
            break;
    }

    cJSON_Delete(json);
    ESP_LOGD(TAG, "Blacklist delete processing Time: %lld ms", (esp_timer_get_time()-start)/1000);
    return ESP_OK;
}

static httpd_uri_t blacklist_delete = {
    .uri       = "/blacklist/delete",
    .method    = HTTP_POST,
    .handler   = blacklist_delete_handler,
    .user_ctx  = ""
};

esp_err_t setup_blacklist_handlers(httpd_handle_t server)
{ 
    ATTEMPT(httpd_register_uri_handler(server, &blacklist))
    ATTEMPT(httpd_register_uri_handler(server, &blacklist_add))
    ATTEMPT(httpd_register_uri_handler(server, &blacklist_delete))
    ATTEMPT(httpd_register_uri_handler(server, &blacklist_json))

    return ESP_OK;
}

esp_err_t teardown_blacklist_handlers(httpd_handle_t server)
{ 
    ATTEMPT(httpd_unregister_uri_handler(server, blacklist.uri, blacklist.method))
    ATTEMPT(httpd_unregister_uri_handler(server, blacklist_add.uri, blacklist_add.method))
    ATTEMPT(httpd_unregister_uri_handler(server, blacklist_delete.uri, blacklist_delete.method))
    ATTEMPT(httpd_unregister_uri_handler(server, blacklist_json.uri, blacklist_json.method))

    return ESP_OK;
}