#include "filesystem.h"
#include "error.h"
#include "events.h"
#include "cJSON.h"
#include "string.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "errno.h"
#include "esp_littlefs.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "FILE";

#define BASE_PATH "/littlefs"
static char base_path[MAX_FILENAME_LENGTH];          // /littlefs/ota_0 or /littlefs/ota_1


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

esp_err_t delete_file(const char* filename)
{
    char path[MAX_FILENAME_LENGTH];
    strcpy(path, base_path);
    strcat(path, filename);
    unlink(filename);

    return ESP_OK;
}

esp_err_t rename_file(const char* before, const char* after)
{
    char b[MAX_FILENAME_LENGTH];
    strcpy(b, base_path);
    strcat(b, before);

    char a[MAX_FILENAME_LENGTH];
    strcpy(a, base_path);
    strcat(a, after);

    rename(b, a);

    return ESP_OK;
}

esp_err_t get_setting(const char* key, char* buffer)
{
    cJSON* json = get_settings_json();
    if( json == NULL)
    {
        log_error(ESP_ERR_NOT_FOUND, "get_settings_json()", __func__, __FILE__);
        return ESP_FAIL;
    }

    cJSON* object = cJSON_GetObjectItem(json, key);
    strcpy(buffer, object->valuestring);
    cJSON_Delete(json);
    return ESP_OK;
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
    ESP_LOGW(TAG, "Saved %s", #SRC );                                           \
    fclose(file);                                                               \
} while(0)

static esp_err_t initialize_files()
{
    ESP_LOGW(TAG, "Copying files from app binary...");

    COPY_EMBED("/app/blacklist/index.html",     blacklist_html);
    COPY_EMBED("/app/settings/index.html",      settings_html);
    COPY_EMBED("/app/index.html",               homepage_html);
    COPY_EMBED("/app/stylesheet.css",           stylesheet_css);
    COPY_EMBED("/app/scripts.js",               app_scripts_js);
    COPY_EMBED("/app/blacklist.txt",            defaultblacklist_txt);
    COPY_EMBED("/app/settings.json",            settings_json);
    COPY_EMBED("/app/404.html",                 404_html);
    COPY_EMBED("/prov/connected/index.html",    connected_html);
    COPY_EMBED("/prov/index.html",              wifi_select_html);
    COPY_EMBED("/prov/scripts.js",              prov_scripts_js);
    COPY_EMBED("/prov/stylesheet.css",          stylesheet_css);

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
    ESP_LOGI(TAG, "Initializing LittleFS...");
    esp_vfs_littlefs_conf_t conf = {
      .base_path = BASE_PATH,
      .partition_label = "littlefs",
      .format_if_mount_failed = true,
      .dont_mount = false
    };
    

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    // assign base_path to either ota_0 or ota_1
    strcpy(base_path, BASE_PATH"/");
    const esp_partition_t *running = esp_ota_get_running_partition();
    strcat(base_path, running->label);
    ESP_LOGI(TAG, "LittleFS base: %s", base_path);

    // check if files have been initialized by checking version.txt
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

    // LittleFS stats
    size_t total = 0, used = 0;
    ret = esp_littlefs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "LittleFS Partition size: total: %d, used: %d", total, used);
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