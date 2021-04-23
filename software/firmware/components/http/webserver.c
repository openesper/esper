#include "webserver.h"
#include "error.h"
#include "events.h"
#include "esp_http_server.h"
#include "sys/stat.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char* TAG = "HTTP";

static const char* app_directory = "/spiffs/application";
static const char* prov_directory = "/spiffs/provisioning";
static const char* index_html = "index.html";

#define MAX_CHUNK_SIZE 1000          // Size of chunks to send
static char chunk[MAX_CHUNK_SIZE];   // buffer for chunks

static httpd_handle_t server; // Handle for HTTP server


static const char* get_filename_ext(const char *filepath) 
{
    const char *dot = strrchr(filepath, '.');
    if(!dot || dot == filepath) return "";
    return dot + 1;
}

static esp_err_t set_content_type(httpd_req_t *req, const char* filepath)
{
    const char* ext = get_filename_ext(filepath);

    if( strcmp(ext, "html") == 0 ){
        httpd_resp_set_type(req, "text/html");
    } else if( strcmp(ext, "css") == 0 ){
        httpd_resp_set_type(req, "text/css");
    } else if( strcmp(ext, "js") == 0 ){
        httpd_resp_set_type(req, "text/javascript");
    } else if( strcmp(ext, "txt") == 0 ){
        httpd_resp_set_type(req, "text/plain");
    } else if( strcmp(ext, "json") == 0 ){
        httpd_resp_set_type(req, "application/json");
    } else if( strcmp(ext, "ico") == 0 ){
        httpd_resp_set_type(req, "image/x-icon");
    } else if( strcmp(ext, "png") == 0 ){
        httpd_resp_set_type(req, "image/png");
    } else if( strcmp(ext, "jpg") == 0 ){
        httpd_resp_set_type(req, "image/jpg");
    }

    return ESP_OK;
}

static esp_err_t http_get(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Request for %s", req->uri);

    // Choose directory
    const char* src_directory = app_directory;
    if( check_bit(PROVISIONING_BIT) )
        src_directory = prov_directory;

    // Build filepath
    char filepath[strlen(src_directory)+HTTPD_MAX_URI_LEN+strlen(index_html)+1+1];
    strcpy(filepath, src_directory);
    strcat(filepath, req->uri);

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
    if( stat(filepath, &s) < 0 )
    {
        ESP_LOGW(TAG, "%s does not exist", filepath);
        httpd_resp_send_404(req);
        return ESP_OK;
    }

    // Open file
    FILE* f = fopen(filepath, "r");
    if (f == NULL)
    {
        ESP_LOGW(TAG, "Unable to open file");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to open file");
        return ESP_OK;
    }

    // Set headers
    httpd_resp_set_status(req, HTTPD_200);
    set_content_type(req, filepath);

    // Send file
    size_t chunksize;
    do {
        chunksize = fread(chunk, 1, MAX_CHUNK_SIZE, f);
        if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
            fclose(f);
            ESP_LOGE(TAG, "File sending failed!");
            httpd_resp_sendstr_chunk(req, NULL);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
            return ESP_FAIL;
        }
    } while(chunksize != 0);

    fclose(f);
    return ESP_OK;
}

esp_err_t start_webserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    // config.max_uri_handlers = 30;
    // config.stack_size = 8192;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    ERROR_CHECK(httpd_start(&server, &config))

    static httpd_uri_t get = {
        .uri       = "*",
        .method    = HTTP_GET,
        .handler   = http_get,
    };
    ATTEMPT(httpd_register_uri_handler(server, &get))

    return ESP_OK;
}

esp_err_t stop_webserver()
{
    ERROR_CHECK(httpd_stop(server));
    return ESP_OK;
}
