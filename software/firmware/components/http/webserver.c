#include "webserver.h"
#include "blacklist.h"
#include "home.h"
#include "settings.h"
#include "connected.h"
#include "wifi_select.h"
#include "captive_portal.h"
#include "error.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "HTTP";

static httpd_handle_t server; // Handle for HTTP server



esp_err_t start_webserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 30;
    config.stack_size = 8192;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    ERROR_CHECK(httpd_start(&server, &config))

    return ESP_OK;
}

esp_err_t start_application_webserver()
{
    ESP_LOGI(TAG, "Starting Application Webserver");
    ERROR_CHECK(setup_homepage_handlers(server))
    ERROR_CHECK(setup_blacklist_handlers(server))
    ERROR_CHECK(setup_settings_handlers(server))
    return ESP_OK;
}

esp_err_t start_provisioning_webserver()
{
    ESP_LOGI(TAG, "Starting Provisioning Webserver");
    ERROR_CHECK(configure_captive_portal_handlers(server))
    ERROR_CHECK(configure_wifi_select_handler(server))
    ERROR_CHECK(configure_connected_handler(server))
    return ESP_OK;
}

esp_err_t stop_provisioning_webserver()
{
    ESP_LOGI(TAG, "Starting Provisioning Webserver");
    ERROR_CHECK(teardown_captive_portal_handlers(server))
    ERROR_CHECK(teardown_wifi_select_handler(server))
    ERROR_CHECK(teardown_connected_handler(server))
    return ESP_OK;
}

esp_err_t stop_webserver()
{
    ERROR_CHECK(httpd_stop(server));
    return ESP_OK;
}