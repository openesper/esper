#include "filesystem.h"
#include "flash.h"
#include "error.h"
#include "events.h"
#include "esp_spiffs.h"
#include "sys/stat.h"
#include "cJSON.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "FILE";


cJSON* get_settings_json()
{

    char buffer[1000+1];
    FILE* settings = fopen("/spiffs/settings.json", "r");
    if( settings == NULL )
    {
        return NULL;
    }

    int bytes_read;
    if( (bytes_read = fread(buffer, 1, 1000, settings)) < 1 )
    {
        return NULL;
    }
    buffer[bytes_read] = '\0';
    fclose(settings);

    return cJSON_Parse(buffer);
}

static init_blacklist_txt()
{
    struct stat s;
    if( stat("/spiffs/blacklist.txt", &s) < 0 )
    {
        ESP_LOGW(TAG, "Creating blacklist.txt...");
        FILE* defaultBlacklist = fopen("/spiffs/defaultblacklist.txt", "r");
        if( defaultBlacklist == NULL )
        {
            log_error(ESP_FAIL, "Could not initialize blacklist.txt, unable to open defaultblacklist.txt");
            return ESP_FAIL;
        }

        FILE* blacklist = fopen("/spiffs/application/blacklist.txt", "w");
        if( blacklist == NULL )
        {
            log_error(ESP_FAIL, "Could not initialize blacklist.txt, unable to open blacklist.txt");
            return ESP_FAIL;
        }

        char buf[1000];
        while( fread(buf, 1, 1000, defaultBlacklist) != 0 )
        {
            fwrite(buf, 1, 1000, blacklist);
        }
    }

    return ESP_OK;
}

static esp_err_t initialize_files()
{

}

esp_err_t init_filesystem()
{
    ESP_LOGI(TAG, "Initializing SPIFFS...");
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 10,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    bool initialized = false;
    get_file_initialization(&initialized);
    if ( !initialized )
    {
        initialize_files();
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ATTEMPT(initialize_files())

    ESP_LOGI(TAG, "Spiffs Partition size: total: %d, used: %d", total, used);
    return ESP_OK;
}