#include "post_handlers.h"
#include "webserver.h"
#include "error.h"
#include "events.h"
// #include "wifi.h"
#include "filesystem.h"
#include "settings.h"
// 
#include "ota.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "lwip/ip4_addr.h"

#include <string>

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
#include "esp_log.h"
static const char* TAG = "HTTP";


esp_err_t settings_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST to %s", req->uri);

    // Determine if content is too large/small
    if (req->content_len > 1024)
        SEND_ERR(req, HTTPD_400_BAD_REQUEST, "Form data too large")

    // receive data
    char data[req->content_len+1] = {};
    httpd_req_recv(req, data, req->content_len);

    char param[255]= {};
    if( httpd_query_key_value(data, "ip", param, sizeof(param)) == ESP_OK )
    {
        ip4_addr_t addr;
        if( ip4addr_aton(param, &addr) > 0)
            setting::write(setting::IP, param);
    }

    if( httpd_query_key_value(data, "url", param, sizeof(param)) == ESP_OK )
    {
        setting::write(setting::HOSTNAME, param);
    }

    if( httpd_query_key_value(data, "dnssrv", param, sizeof(param)) == ESP_OK )
    {
        ip4_addr_t addr;
        if( ip4addr_aton(param, &addr) > 0)
            setting::write(setting::DNS_SRV, param);
    }


    if( httpd_query_key_value(data, "updatesrv", param, sizeof(param)) == ESP_OK )
    {
        setting::write(setting::UPDATE_SRV, param);
    }

    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/settings?status=success");
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

static httpd_uri_t settings = {
    .uri       = "/settings",
    .method    = HTTP_POST,
    .handler   = settings_post_handler,
    .user_ctx  = NULL
};


esp_err_t toggleblock_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST to %s", req->uri);

    bool blocking = !setting::read_bool(setting::BLOCK);
    setting::write(setting::BLOCK, blocking);
    // if( setting::write(setting::BLOCK, blocking) != ESP_OK )
    // {
    //     SEND_ERR(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error writing to settings.json")
    // }

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_sendstr(req, (setting::read_bool(setting::BLOCK) ? "true":"false") );

    return ESP_OK;
}

static httpd_uri_t toggleblock = {
    .uri       = "/toggleblock",
    .method    = HTTP_POST,
    .handler   = toggleblock_handler,
    .user_ctx  = NULL
};

esp_err_t updatefirmware_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST to %s", req->uri);
    try {
        update_firmware();
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_set_status(req, HTTPD_200);
        httpd_resp_send(req, NULL, 0 );
    } catch (std::string e) {
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_set_status(req, HTTPD_500);
        httpd_resp_sendstr(req, e.c_str() );
    }

    return ESP_OK;
}

static httpd_uri_t updatefirmware = {
    .uri       = "/updatefirmware",
    .method    = HTTP_POST,
    .handler   = updatefirmware_handler,
    .user_ctx  = NULL
};


static void restartCallback(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "restarting");
    esp_restart();
}

esp_err_t restart_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST to %s", req->uri);
    xTimerHandle restartTimer = xTimerCreate("Check for update", pdMS_TO_TICKS(100), pdTRUE, (void*)0, restartCallback);
    xTimerStart(restartTimer, 0);

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_send(req, NULL, 0 );
    return ESP_OK;
}

static httpd_uri_t restart = {
    .uri       = "/restart",
    .method    = HTTP_POST,
    .handler   = restart_handler,
    .user_ctx  = NULL
};


esp_err_t register_post_handlers(httpd_handle_t server)
{
    ATTEMPT(httpd_register_uri_handler(server, &settings))
    ATTEMPT(httpd_register_uri_handler(server, &toggleblock))
    ATTEMPT(httpd_register_uri_handler(server, &updatefirmware))
    ATTEMPT(httpd_register_uri_handler(server, &restart))

    return ESP_OK;
}


