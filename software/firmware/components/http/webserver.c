#include "webserver.h"
#include "post_handlers.h"
#include "error.h"
#include "events.h"
#include "filesystem.h"
#include "esp_http_server.h"
// #include "esp_https_server.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char* TAG = "HTTP";

static const char* app_directory = "/app";
static const char* prov_directory = "/prov";
static const char* index_html = "index.html";

#define MAX_CHUNK_SIZE 1000          // Size of chunks to send
static char chunk[MAX_CHUNK_SIZE];   // buffer for chunks

static httpd_handle_t server; // Handle for HTTP server
static httpd_handle_t redirect_server; // Handle for HTTP server


static esp_err_t redirect_handler(httpd_req_t* req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_status(req, "301 Moved Permanently");
    httpd_resp_set_hdr(req, "Location", "http://192.168.2.12:80/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

esp_err_t start_https_redirect_server()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.server_port = 443;

    ESP_LOGI(TAG, "Starting redirect server on port: '%d'", config.server_port);
    ATTEMPT(httpd_start(&redirect_server, &config))

    static httpd_uri_t get = {
        .uri       = "*",
        .method    = HTTP_GET,
        .handler   = redirect_handler,
    };
    ATTEMPT(httpd_register_uri_handler(server, &get))

    return ESP_OK;
}

static const char* get_filename_ext(const char *filepath) 
{
    const char *dot = strrchr(filepath, '.');
    if(!dot || dot == filepath) return "";
    return dot + 1;
}

static esp_err_t set_content_type(httpd_req_t *req, const char* filepath)
{
    const char* ext = get_filename_ext(filepath);

    char* type = "text/html";
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
    }

    return httpd_resp_set_type(req, type);
}

static esp_err_t send_file(httpd_req_t *req, const char* filepath)
{
    FILE* f = open_file(filepath, "r");
    if (f == NULL)
    {
        ESP_LOGI(TAG, "Unable to open %s", filepath);
        return ESP_FAIL;
    }

    size_t chunksize;
    do {
        chunksize = fread(chunk, 1, MAX_CHUNK_SIZE, f);
        if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
            ESP_LOGI(TAG, "Error sending %s", filepath);
            fclose(f);
            return ESP_FAIL;
        }
    } while(chunksize != 0);

    fclose(f);
    return ESP_OK;
}

static esp_err_t get_handler(httpd_req_t *req)
{
    // Remove query parameters from uri
    char uri[HTTPD_MAX_URI_LEN];
    memcpy(uri, req->uri, strcspn(req->uri, "?"));
    uri[strcspn(req->uri, "?")] = '\0';
    ESP_LOGI(TAG, "Request for %s", uri);

    // Choose directory
    const char* src_directory = app_directory;
    if( check_bit(PROVISIONING_BIT) )
        src_directory = prov_directory;

    // Build filepath
    char filepath[strlen(src_directory)+HTTPD_MAX_URI_LEN+strlen(index_html)+1+1];
    strcpy(filepath, src_directory);
    strcat(filepath, uri);

    // See file extension exists, otherwise assume 
    // it's a directory and look for index.html
    struct stat s;
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
    if( stat_file(filepath, &s) < 0 )
    {
        // Redirect to "/" if currently provisioning
        if( check_bit(PROVISIONING_BIT) )
        {
            ESP_LOGD(TAG, "Redirecting to /");
            httpd_resp_set_type(req, "text/html");
            httpd_resp_set_status(req, "302 Found");
            httpd_resp_set_hdr(req, "Location", "/");
            httpd_resp_send(req, NULL, 0);
        }
        else // Send 404 page
        {
            ESP_LOGD(TAG, "Sending 404.html");
            httpd_resp_set_status(req, HTTPD_404);
            httpd_resp_set_type(req, "text/html");
            if( send_file(req, "/app/404.html") != ESP_OK )
            {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to send 404");
            }  
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

esp_err_t start_webserver()
{
    //start_https_redirect_server();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    ATTEMPT(httpd_start(&server, &config))

    static httpd_uri_t get = {
        .uri       = "*",
        .method    = HTTP_GET,
        .handler   = get_handler,
    };
    ATTEMPT(httpd_register_uri_handler(server, &get))

    static httpd_uri_t submitauth = {
        .uri       = "/submitauth",
        .method    = HTTP_POST,
        .handler   = submitauth_handler,
    };
    ATTEMPT(httpd_register_uri_handler(server, &submitauth))

    static httpd_uri_t scan = {
        .uri       = "/scan",
        .method    = HTTP_POST,
        .handler   = scan_handler,
    };
    ATTEMPT(httpd_register_uri_handler(server, &scan))

        static httpd_uri_t finish = {
        .uri       = "/finish",
        .method    = HTTP_POST,
        .handler   = finish_handler,
    };
    ATTEMPT(httpd_register_uri_handler(server, &finish))

    return ESP_OK;
}

esp_err_t stop_webserver()
{
    ATTEMPT(httpd_stop(server));
    return ESP_OK;
}
