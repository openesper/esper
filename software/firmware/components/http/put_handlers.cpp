#include "put_handlers.h"
#include "error.h"
#include "events.h"
#include "lists.h"

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
#include "esp_log.h"
static const char* TAG = "HTTP";


esp_err_t blacklist_put_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "PUT Request for %s", req->uri);
    char uri[HTTPD_MAX_URI_LEN];
    strcpy(uri, req->uri);

    strtok(uri, "/");
    char* hostname = strtok(NULL, "/");
    
    if( hostname == NULL )
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty hostname");
        return ESP_OK;
    }
    else if( strlen(hostname) > MAX_URL_LENGTH )
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Hostname too long");
        return ESP_OK;
    }

    ESP_LOGD(TAG, "Adding %s", hostname);
    esp_err_t err = add_to_blacklist(hostname); 
    if( err == URL_ERR_INVALID_URL )
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid hostname");
        return ESP_OK;
    }
    else if( err != ESP_OK )
    {
        httpd_resp_send_500(req);
        return ESP_OK;
    }

    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, hostname);
    return ESP_OK;
}

static httpd_uri_t blacklistadd = {
    .uri       = "/blacklist/*",
    .method    = HTTP_PUT,
    .handler   = blacklist_put_handler,
    .user_ctx  = NULL
};


esp_err_t register_put_handlers(httpd_handle_t server)
{
    ATTEMPT(httpd_register_uri_handler(server, &blacklistadd))

    return ESP_OK;
}