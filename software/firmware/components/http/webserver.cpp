#include "webserver.h"
#include "post_handlers.h"
#include "get_handlers.h"
#include "put_handlers.h"
#include "delete_handlers.h"
#include "error.h"
#include "events.h"
#include "filesystem.h"
#include "esp_http_server.h"
// #include "esp_https_server.h"

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
#include "esp_log.h"
static const char* TAG = "HTTP";

static httpd_handle_t server; // Handle for HTTP server
// static httpd_handle_t redirect_server; // Handle for HTTP server


esp_err_t start_webserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    ATTEMPT(httpd_start(&server, &config))
    ATTEMPT(register_get_handlers(server))
    ATTEMPT(register_post_handlers(server))
    ATTEMPT(register_put_handlers(server))
    ATTEMPT(register_delete_handlers(server))

    return ESP_OK;
}

esp_err_t stop_webserver()
{
    ATTEMPT(httpd_stop(server));
    return ESP_OK;
}


// static esp_err_t redirect_handler(httpd_req_t* req)
// {
//     httpd_resp_set_type(req, "text/html");
//     httpd_resp_set_status(req, "301 Moved Permanently");
//     httpd_resp_set_hdr(req, "Location", "http://192.168.2.12:80/");
//     httpd_resp_send(req, NULL, 0);
//     return ESP_OK;
// }

// esp_err_t start_https_redirect_server()
// {
//     httpd_config_t config = HTTPD_DEFAULT_CONFIG();
//     config.uri_match_fn = httpd_uri_match_wildcard;
//     config.server_port = 443;

//     ESP_LOGI(TAG, "Starting redirect server on port: '%d'", config.server_port);
//     ATTEMPT(httpd_start(&redirect_server, &config))

//     static httpd_uri_t get = {
//         .uri       = "*",
//         .method    = HTTP_GET,
//         .handler   = redirect_handler,
//     };
//     ATTEMPT(httpd_register_uri_handler(server, &get))

//     return ESP_OK;
// }
