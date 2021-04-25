#include "filesystem.h"
#include "error.h"
#include "events.h"
#include "esp_spiffs.h"
#include "cJSON.h"
#include "string.h"
// #include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "errno.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "FILE";

static char base_path[MAX_FILENAME_LENGTH];          // /spiffs/ota_0 or /spiffs/ota_1


FILE* open_file(const char* filename, const char* mode){
    if( filename == NULL)
        return NULL;

    char path[MAX_FILENAME_LENGTH];
    strcpy(path, base_path);
    strcat(path, filename);

    ESP_LOGD(TAG, "Opening %s", path);
    return fopen(path, mode);
}

int stat_file(const char* filename, struct stat* s)
{
    if( filename == NULL)
        return 0;

    char path[MAX_FILENAME_LENGTH];
    strcpy(path, base_path);
    strcat(path, filename);

    ESP_LOGD(TAG, "Stating %s", path);
    return stat(path, s);
}

cJSON* get_settings_json()
{
    char buffer[1000+1];
    FILE* settings = open_file("/app/settings.json", "r");
    if( settings == NULL )
        return NULL;


    int bytes_read;
    if( (bytes_read = fread(buffer, 1, 1000, settings)) < 1 )
        return NULL;
    
    buffer[bytes_read] = '\0';
    fclose(settings);

    return cJSON_Parse(buffer);
}

#define COPY_EMBED(DEST, SRC) do {                                              \
    FILE* file = open_file(DEST, "w");                                          \
    if( file == NULL )                                                          \
    {                                                                           \
        ESP_LOGE(TAG, "Could not open %s (%d)", DEST, errno);                   \
        return ESP_FAIL;                                                        \
    }                                                                           \
    extern const unsigned char  start_##SRC[] asm("_binary_" #SRC "_start");    \
    extern const unsigned char  end_##SRC[]   asm("_binary_" #SRC "_end");      \
    const size_t  size = (end_##SRC -  start_##SRC);                            \
    if( fwrite(start_##SRC, 1, size, file)  < size)                             \
    {                                                                           \
        ESP_LOGE(TAG, "Could not copy %s (%d)", #SRC, errno);                   \
        return ESP_FAIL;                                                        \
    }                                                                           \
    ESP_LOGD(TAG, "Copied %s", #SRC );                                          \
    fclose(file);                                                               \
} while(0)

static esp_err_t initialize_files()
{
    ESP_LOGI(TAG, "Initializing files...");

    COPY_EMBED("/app/blacklist/index.html",     blacklist_html);
    COPY_EMBED("/app/settings/index.html",      settings_html);
    COPY_EMBED("/app/index.html",               homepage_html);
    COPY_EMBED("/app/stylesheet.css",           stylesheet_css);
    COPY_EMBED("/app/scripts.js",               app_scripts_js);
    COPY_EMBED("/app/blacklist.txt",            defaultblacklist_txt);
    COPY_EMBED("/app/settings.json",            settings_json);
    COPY_EMBED("/prov/connected/index.html",    connected_html);
    COPY_EMBED("/prov/index.html",              wifi_select_html);
    COPY_EMBED("/prov/scripts.js",              prov_scripts_js);
    COPY_EMBED("/prov/stylesheet.css",          stylesheet_css);
    COPY_EMBED("/prov/hotspot-detect.html",     hotspot_detect_html);
    COPY_EMBED("/prov/connecttest.txt",         connecttest_txt);

    FILE* v = open_file("/version.txt", "w");
    if( v == NULL )
        return ESP_FAIL;

    esp_app_desc_t desc;
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_get_partition_description(running, &desc);
    if( fwrite(desc.version, 1, strlen(desc.version)+1, v) < strlen(desc.version)+1)
        return ESP_FAIL;
        
    fclose(v);

    return ESP_OK;
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
    

    strcpy(base_path, "/spiffs/");
    const esp_partition_t *running = esp_ota_get_running_partition();
    strcat(base_path, running->label);
    ESP_LOGI(TAG, "Spiffs base: %s", base_path);

    FILE* ver = open_file("/version.txt", "r");
    if( ver == NULL ){
        ATTEMPT(initialize_files())
    }else{
        char buf[10];
        fread(buf, 1, 10, ver);

        esp_app_desc_t running_app_info;
        esp_ota_get_partition_description(running, &running_app_info);

        if( strcmp(buf, running_app_info.version) )
            ATTEMPT(initialize_files())
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Spiffs Partition size: total: %d, used: %d", total, used);
    return ESP_OK;
}

// static esp_err_t print_file(const char* filename)
// {
//     char buffer[100+1];
//     FILE* f = open_file(filename, "r");
//     if( f == NULL )
//     {
//         ESP_LOGE(TAG, "Could not open %s", filename);
//         return ESP_FAIL;
//     }

//     int bytes_read;
//     if( (bytes_read = fread(buffer, 1, 100, f)) < 1 )
//     {
//         ESP_LOGE(TAG, "Could not open %s", filename);
//         return ESP_FAIL;
//     }
    
//     buffer[bytes_read] = '\0';
//     ESP_LOGI(TAG, "%s\n%s", filename, buffer);
//     fclose(f);

//     return ESP_OK;
// }