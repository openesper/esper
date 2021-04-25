#include "home.h"
#include "error.h"
#include "logging.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "HTTP";

/**
  * @brief Response to log.json GET request
  */
static esp_err_t log_json_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for %s", req->uri);
    long start = esp_timer_get_time();

    // Default values if query string doesn't exist
    uint32_t size = 10;
    uint32_t page = 0;

    // Check that the query len is appropriate
    uint32_t query_len = httpd_req_get_url_query_len(req) + 1;
    if (query_len > 25) // 25 is an arbitrary number that is larger than the request should ever be, assuming a log file size of 1MB
    {
        ESP_LOGW(TAG, "Query too large");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Query too large");
        return ESP_OK;
    }
    else if (query_len > 1)
    {
        // check that query string is valid
        char query_buf[query_len];
        if ( httpd_req_get_url_query_str(req, query_buf, query_len) != ESP_OK)
        {
            ESP_LOGW(TAG, "Invalid query string");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid query string");
            return ESP_OK;
        }

        // retreive size and page from query string
        char s[5], p[10];
        if ( httpd_query_key_value(query_buf, "size", s, sizeof(s)) != ESP_OK || sscanf(s, "%i", &size) < 1 ||
             httpd_query_key_value(query_buf, "page", p, sizeof(p)) != ESP_OK || sscanf(p, "%i", &page) < 1) 
        {
            ESP_LOGW(TAG, "Invalid parameters");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid parameters");
            return ESP_OK;
        }

        if (size > 50)
            size = 50;
    }

    // build log json
    cJSON* json = cJSON_CreateObject();
    if( build_log_json(json, size, page) != ESP_OK)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error building json");
        cJSON_Delete(json);
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json"); 
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_sendstr(req, cJSON_Print(json));
    cJSON_Delete(json);

    ESP_LOGD(TAG, "Log.csv processing time: %lld ms", (esp_timer_get_time()-start)/1000);
    return ESP_OK;
}

static httpd_uri_t log_json = {
    .uri       = "/log.json",
    .method    = HTTP_GET,
    .handler   = log_json_get_handler,
    .user_ctx  = ""
};

/**
  * @brief Response for homepage GET request
  */
static esp_err_t homepage_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for %s", req->uri);
    long start = esp_timer_get_time();

    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    extern const unsigned char  home_html_start[] asm("_binary_home_html_start");
    extern const unsigned char  home_html_end[]   asm("_binary_home_html_end");
    const size_t  home_html_size = (home_html_end -  home_html_start);
    httpd_resp_send(req, (const char *)home_html_start,  home_html_size);
    ESP_LOGD(TAG, "Homepage.html processing Time: %lld ms", (esp_timer_get_time()-start)/1000);

    return ESP_OK;
}

static httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = homepage_get_handler,
    .user_ctx  = ""
};

static httpd_uri_t homepage = {
    .uri       = "/homepage",
    .method    = HTTP_GET,
    .handler   = homepage_get_handler,
    .user_ctx  = ""
};

/**
  * @brief Setup all handlers needed for homepage
  */
esp_err_t setup_homepage_handlers(httpd_handle_t server)
{
    ATTEMPT(httpd_register_uri_handler(server, &root))
    ATTEMPT(httpd_register_uri_handler(server, &homepage))
    ATTEMPT(httpd_register_uri_handler(server, &log_json))

    return ESP_OK;
}

/**
  * @brief Teardown all homepage handlers
  */
esp_err_t teardown_homepage_handlers(httpd_handle_t server)
{
    ATTEMPT(httpd_unregister_uri_handler(server, root.uri, root.method))
    ATTEMPT(httpd_unregister_uri_handler(server, homepage.uri, homepage.method))
    ATTEMPT(httpd_unregister_uri_handler(server, log_json.uri, log_json.method))

    return ESP_OK;
}