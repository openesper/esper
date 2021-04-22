#include "captive_portal.h"
#include "error.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "HTTP";


static esp_err_t apple_captive_portal_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for /hotspot-detect.html");
    httpd_resp_set_type(req, "text/plain; charset=UTF-8");
    httpd_resp_send(req, "", 0);

    return ESP_OK;
}

static httpd_uri_t apple_captive_portal = {
    .uri       = "/hotspot-detect.html",
    .method    = HTTP_GET,
    .handler   = apple_captive_portal_handler,
    .user_ctx  = ""
};

static esp_err_t windows10_captive_portal_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for /connecttest.txt");
    httpd_resp_set_type(req, "text/plain; charset=UTF-8");
    httpd_resp_send(req, "Microsoft Connect Test", strlen("Microsoft Connect Test"));

    return ESP_OK;
}

static httpd_uri_t windows10_captive_portal = {
    .uri       = "/connecttest.txt",
    .method    = HTTP_GET,
    .handler   = windows10_captive_portal_handler,
    .user_ctx  = ""
};

esp_err_t configure_captive_portal_handlers(httpd_handle_t server)
{
    ERROR_CHECK(httpd_register_uri_handler(server, &apple_captive_portal))
    ERROR_CHECK(httpd_register_uri_handler(server, &windows10_captive_portal))

    return ESP_OK;
}

esp_err_t teardown_captive_portal_handlers(httpd_handle_t server)
{
    ERROR_CHECK(httpd_unregister_uri_handler(server, apple_captive_portal.uri, apple_captive_portal.method))
    ERROR_CHECK(httpd_unregister_uri_handler(server, windows10_captive_portal.uri, windows10_captive_portal.method))

    return ESP_OK;
}