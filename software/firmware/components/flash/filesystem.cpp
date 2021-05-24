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

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
#include "esp_log.h"
static const char *TAG = "FILE";

#define BASE_PATH "/fs"
static std::string curr_dir(BASE_PATH);
static std::string prev_dir(BASE_PATH);

using namespace fs;
file::file(std::string path, const char* mode) {
    fpath = path;
    handle = fopen(path.c_str(), mode);
    if( handle == NULL )
    {
        THROWE(errno, "Failed to open file (%s)", strerror(errno))
    }
}

file::~file() {
    fclose(handle);
}

size_t file::read(void* buffer, size_t size, size_t count)
{
    size_t read = 0;
    read = fread(buffer, size, count, handle);
    if( ferror(handle) )
    {
        THROWE(errno, "Error reading %s", fpath.c_str());
    }
    return read;
}

size_t file::write(const void* buffer, size_t size, size_t count)
{
    size_t written = 0;
    written = fwrite(buffer, size, count, handle);
    if( written < count )
    {
        THROWE(errno, "Error writing to %s, %s", fpath.c_str(), strerror(errno));
    }
    return written;
}

file fs::open(std::string path, const char* mode)
{
    return file(curr_dir+path, mode);
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
            THROWE(errno, "Error checking status of %s, %s", path.c_str(), strerror(errno));
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
        THROWE(errno, "Error checking status of %s, %s", path.c_str(), strerror(errno));
    }
    return s;
}

void fs::unlink(std::string path)
{
    std::string full_path = curr_dir + path;
    if( ::unlink(full_path.c_str()) < 0)
    {
        THROWE(errno, "Error unlinking %s, %s", path.c_str(), strerror(errno));
    }
}

void fs::rename(std::string before, std::string after)
{
    std::string full_path = curr_dir + before;
    std::string new_path = curr_dir + after;

    if( ::rename(full_path.c_str(), new_path.c_str()) )
    {
        THROWE(errno, "Error renameing %s, %s", before.c_str(), strerror(errno));
    }
}

// asm() only accepts string literals, so macro must be used
#define COPY_EMBED(DEST, SRC) {                                                 \
    extern const unsigned char  start_##SRC[] asm("_binary_" #SRC "_start");    \
    extern const unsigned char  end_##SRC[]   asm("_binary_" #SRC "_end");      \
    const size_t  size = (end_##SRC -  start_##SRC);                            \
    file f = fs::open(DEST, "w");                                                   \
    f.write(start_##SRC, 1, size);                                              \
    ESP_LOGW(TAG, "Saved %s", #SRC );                                           \
}

static void copy_from_binary()
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
    struct stat s;
    if( ::stat(std::string(prev_dir+"/settings.json").c_str(), &s) == 0)
    {
        std::string old_path = prev_dir + "/settings.json";
        std::string new_path = curr_dir + "/settings.json";
        if( ::rename(old_path.c_str(), new_path.c_str()) != 0 )
        {
            THROWE(errno, "Error renameing %s, %s", old_path.c_str(), strerror(errno))
        }

        old_path = prev_dir + "/blacklist.txt";
        new_path = curr_dir + "/blacklist.txt";
        if( ::rename(old_path.c_str(), new_path.c_str()) != 0 )
        {
            THROWE(errno, "Error renameing %s, %s", old_path.c_str(), strerror(errno))
        }

        setting::load_settings();
    }
    else
    {
        ESP_LOGW(TAG, "Settings & blacklist not found, copying defaults...");
        COPY_EMBED("/settings.json", defaultsettings_json);
        COPY_EMBED("/blacklist.txt", defaultblacklist_txt);
        setting::load_settings();
#ifdef CONFIG_WIFI_ENABLE
        setting::write(setting::SSID, CONFIG_WIFI_SSID);
        setting::write(setting::PASSWORD, CONFIG_WIFI_PASSWORD);
#endif
    }
    // Save current version
    esp_app_desc_t desc;
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_get_partition_description(running, &desc);
    setting::write(setting::VERSION, desc.version);
}

void reset_fs()
{
    fs::unlink("/settings.json");
    fs::unlink("/blacklist.txt");
    esp_restart();
}

void init_fs()
{
    ESP_LOGI(TAG, "Initializing LittleFS...");

    esp_vfs_littlefs_conf_t conf = {
      .base_path = BASE_PATH,
      .partition_label = "spiffs", // *waves hand* this is not the filesystem you are looking for
      .format_if_mount_failed = true,
      .dont_mount = false
    };
    TRY(esp_vfs_littlefs_register(&conf))

    // Get current & previous/next partition
    const esp_partition_t *running = esp_ota_get_running_partition();
    curr_dir += "/" + std::string(running->label);
    ESP_LOGV(TAG, "Current base: %s", curr_dir.c_str());

    const esp_partition_t *prev_partition = esp_ota_get_next_update_partition(NULL);
    prev_dir += "/" + std::string(prev_partition->label);
    ESP_LOGV(TAG, "Previous base: %s", prev_dir.c_str());

    try {
        setting::load_settings();
    } catch (const Err& e) {
        ESP_LOGE(TAG, "Settings.json not found, copying files from binary");
        copy_from_binary();
    }
}

