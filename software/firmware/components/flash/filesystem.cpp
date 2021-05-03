#include "filesystem.h"
#include "error.h"
#include "events.h"
#include "settings.h"
#include "string.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "errno.h"
#include "esp_littlefs.h"

#include <string>

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "FILE";

#define BASE_PATH "/fs"
static std::string curr_dir(BASE_PATH);
static std::string prev_dir(BASE_PATH);

fs::file::file(std::string path, const char* mode) {
    fpath = path;
    handle = fopen(path.c_str(), mode);
    if( handle == NULL )
    {
        THROW("Failed to open file (%s)", strerror(errno))
    }
}

fs::file::~file() {
    fclose(handle);
}

size_t fs::file::read(void* buffer, size_t size, size_t count)
{
    size_t read = 0;
    read = fread(buffer, size, count, handle);
    if( ferror(handle) )
    {
        THROW("Error reading %s", fpath.c_str());
    }
    return read;
}

size_t fs::file::write(const void* buffer, size_t size, size_t count)
{
    size_t written = 0;
    written = fwrite(buffer, size, count, handle);
    if( written < count )
    {
        THROW("Error writing to %s, %s", fpath.c_str(), strerror(errno));
    }
    return written;
}

fs::file fs::open(std::string path, const char* mode)
{
    return fs::file(curr_dir+path, mode);
}

bool fs::exists(std::string path)
{
    struct stat s;
    std::string full_path = curr_dir + path;
    if( stat(full_path.c_str(), &s) != 0 )
    {
        if( errno == ENOENT )
        {
            return false;
        }
        else
        {
            THROW("Error checking status of %s, %s", path.c_str(), strerror(errno));
        }
    }
    else
    {
        return true;
    }
}

struct stat fs::stat(std::string path)
{
    struct stat s;
    std::string full_path = curr_dir + path;
    if( stat(full_path.c_str(), &s) != 0 )
    {
        THROW("Error checking status of %s, %s", path.c_str(), strerror(errno));
    }
    return s;
}

void fs::unlink(std::string path)
{
    std::string full_path = curr_dir + path;
    if( ::unlink(full_path.c_str()) < 0)
    {
        THROW("Error unlinking %s, %s", path.c_str(), strerror(errno));
    }
}

void fs::rename(std::string before, std::string after)
{
    std::string full_path = curr_dir + before;
    std::string new_path = curr_dir + after;

    if( ::rename(full_path.c_str(), new_path.c_str()) )
    {
        THROW("Error renameing %s, %s", before.c_str(), strerror(errno));
    }
}

// asm() only accepts string literals, so macro must be used
#define COPY_EMBED(DEST, SRC) {                                                 \
    extern const unsigned char  start_##SRC[] asm("_binary_" #SRC "_start");    \
    extern const unsigned char  end_##SRC[]   asm("_binary_" #SRC "_end");      \
    const size_t  size = (end_##SRC -  start_##SRC);                            \
    file f = open(DEST, "w");                                                   \
    f.write(start_##SRC, 1, size);                                              \
    ESP_LOGW(TAG, "Saved %s", #SRC );                                           \
}

static void init_fs()
{
    using namespace fs;                                         
    ESP_LOGW(TAG, "Copying files from app binary...");
    COPY_EMBED("/blacklist/index.html",     blacklist_html);
    COPY_EMBED("/settings/index.html",      settings_html);
    COPY_EMBED("/index.html",               homepage_html);
    COPY_EMBED("/stylesheet.css",           stylesheet_css);
    COPY_EMBED("/scripts.js",               app_scripts_js);
    COPY_EMBED("/404.html",                 404_html);

    // copy previous settings.json & blacklist, if they exist
    if( exists(prev_dir+"/settings.json") )
    {
        std::string old_path = prev_dir + "/settings.json";
        std::string new_path = curr_dir + "/settings.json";
        if( ::rename(old_path.c_str(), new_path.c_str()) != 0 )
        {
            THROW("Error renameing %s, %s", old_path.c_str(), strerror(errno));
        }

        old_path = prev_dir + "/blacklist.txt";
        new_path = curr_dir + "/blacklist.txt";
        if( ::rename(old_path.c_str(), new_path.c_str()) != 0 )
        {
            THROW("Error renameing %s, %s", old_path.c_str(), strerror(errno));
        }

        sett::load_settings();
    }
    else
    {
        ESP_LOGW(TAG, "Settings & blacklist not found, copying defaults...");
        COPY_EMBED("/settings.json", defaultsettings_json);
        COPY_EMBED("/blacklist.txt", defaultblacklist_txt);
        sett::load_settings();
        sett::write(sett::SSID, CONFIG_WIFI_SSID);
        sett::write(sett::PASSWORD, CONFIG_WIFI_PASSWORD);
    }
    // Save current version
    esp_app_desc_t desc;
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_get_partition_description(running, &desc);
    sett::write(sett::VERSION, desc.version);
}

void fs::init()
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
            THROW("Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            THROW("Failed to find LittleFS partition");
        } else {
            THROW("Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
    }

    // Get current & previous/next partition
    const esp_partition_t *running = esp_ota_get_running_partition();
    curr_dir += "/" + std::string(running->label);
    ESP_LOGI(TAG, "Current base: %s", curr_dir.c_str());

    const esp_partition_t *prev_partition = esp_ota_get_next_update_partition(NULL);
    prev_dir += "/" + std::string(prev_partition->label);
    ESP_LOGI(TAG, "Previous base: %s", prev_dir.c_str());

    // check if files have already been copied from app binary
    try {
        sett::load_settings();
    } catch(std::string e) {
        ESP_LOGE(TAG, "Copying files from binary");
        init_fs();
    }

    // LittleFS stats
    size_t total = 0, used = 0;
    ret = esp_littlefs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        THROW("Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "LittleFS Partition size: total: %d, used: %d", total, used);
}

