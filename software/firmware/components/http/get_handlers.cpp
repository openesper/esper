#include "webserver.h"
#include "error.h"
#include "events.h"
#include "filesystem.h"
#include "datetime.h"
#include "dns/logging.h"
#include "esp_http_server.h"
#include "lwip/inet.h"

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#endif
#include "esp_log.h"
static const char* TAG = "HTTP";

static const char* index_html = "index.html";

#define MAX_CHUNK_SIZE 1000          // Size of chunks to send
static char chunk[MAX_CHUNK_SIZE];   // buffer for chunks

static const char* get_filename_ext(const char *filepath) 
{
    const char *dot = strrchr(filepath, '.');
    if(!dot || dot == filepath) return "";
    return dot + 1;
}

static esp_err_t set_content_type(httpd_req_t *req, const char* filepath)
{
    const char* ext = get_filename_ext(filepath);

    const char* type;
    if( strcmp(ext, "html") == 0 ){
        type = "text/html";
    } else if( strcmp(ext, "css") == 0 ){
        type = "text/css";
    } else if( strcmp(ext, "js") == 0 ){
        type = "text/javascript";
    } else if( strcmp(ext, "txt") == 0 ){
        type = "text/plain";
    } else if( strcmp(ext, "json") == 0 ){
        type = "application/json";
    } else if( strcmp(ext, "ico") == 0 ){
        type = "image/x-icon";
    } else if( strcmp(ext, "png") == 0 ){
        type = "image/png";
    } else if( strcmp(ext, "jpg") == 0 ){
        type = "image/jpg";
    } else {
        type = "text/html";
    }

    return httpd_resp_set_type(req, type);
}

static esp_err_t send_file(httpd_req_t *req, const char* filepath)
{
    try{
        fs::file f = fs::open(filepath, "r");

        size_t chunksize;
        do {
            chunksize = f.read(chunk, 1, MAX_CHUNK_SIZE);
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                ESP_LOGE(TAG, "Error sending %s", filepath);
                return ESP_FAIL;
            }
        } while(chunksize != 0);
    }catch(const Err& e){
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t get_handler(httpd_req_t *req)
{
    // Remove query parameters from uri
    char uri[HTTPD_MAX_URI_LEN];
    memcpy(uri, req->uri, strcspn(req->uri, "?"));
    uri[strcspn(req->uri, "?")] = '\0';
    ESP_LOGI(TAG, "Request for %s", uri);

    // Build filepath
    char filepath[HTTPD_MAX_URI_LEN+strlen(index_html)+1+1];
    strcpy(filepath, uri);

    // See file extension exists, otherwise assume 
    // it's a directory and look for index.html
    const char* ext = get_filename_ext(filepath);
    if( ext[0] == '\0' )
    {
        if(filepath[strlen(filepath)-1] != '/')
        {
            strcat(filepath, "/");
        }

        strcat(filepath, index_html);
    }

    // Check if file exists
    if( !fs::exists(filepath) )
    {
        ESP_LOGD(TAG, "Sending 404.html");
        httpd_resp_set_status(req, HTTPD_404);
        httpd_resp_set_type(req, "text/html");
        if( send_file(req, "/404.html") != ESP_OK )
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to send 404");
        }  
        return ESP_OK;
    }

    // Set headers & send
    httpd_resp_set_status(req, HTTPD_200);
    set_content_type(req, filepath);
    if( send_file(req, filepath) != ESP_OK )
    {
        ESP_LOGE(TAG, "Unable to send file");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to send file");
    }   

    return ESP_OK;
}

static httpd_uri_t get = {
    .uri       = "*",
    .method    = HTTP_GET,
    .handler   = get_handler,
    .user_ctx  = NULL
};

esp_err_t querylog_csv_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for %s", req->uri);

    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "[");

    char str[500];
    while( get_log_size() != 0 )
    {
        Log_Entry entry = get_entry();
        sprintf(str, "{ \"time\":\"%s\", \"domain\":\"%s\", \"client\":\"%s\", \"blocked\":%d}", get_time_str(entry.time).c_str(), entry.domain.c_str(), inet_ntoa(entry.client), entry.blocked);
        if( get_log_size() != 0 )
        {
            strcat(str, ",\n");
        }
        else
        {
            strcat(str, "]");
        }

        ESP_LOGV(TAG, "Sending %s", str);
        if( httpd_resp_sendstr_chunk(req, str) != ESP_OK )
        {
            httpd_resp_send_500(req);
            return ESP_OK;
        }
    }

    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}

static httpd_uri_t querylog = {
    .uri       = "/querylog.json",
    .method    = HTTP_GET,
    .handler   = querylog_csv_handler,
    .user_ctx  = NULL
};


esp_err_t register_get_handlers(httpd_handle_t server)
{
    if( !check_bit(PROVISIONING_BIT) )
    {
        ATTEMPT(httpd_register_uri_handler(server, &querylog))
    }
    ATTEMPT(httpd_register_uri_handler(server, &get))
    
    return ESP_OK;
}