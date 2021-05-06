#include "settings.h"
#include "error.h"
#include "events.h"
#include "filesystem.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "cJSON.h"

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
#include "esp_log.h"
static const char *TAG = "SETTINGS";

static SemaphoreHandle_t lock;
static cJSON* settings;

static const char* get_key(sett::Key s)
{
    switch( s )
    {
        case sett::IP:
            return "ip";
        case sett::NETMASK:
            return "netmask";
        case sett::GATEWAY:
            return "gateway";
        case sett::SSID:
            return "ssid";
        case sett::PASSWORD:
            return "password";
        case sett::UPDATE_SRV:
            return "update_srv";
        case sett::HOSTNAME:
            return "url";
        case sett::DNS_SRV:
            return "dns_srv";
        case sett::VERSION:
            return "version";
        case sett::BLOCK:
            return "blocking";
        case sett::UPDATE_AVAILABLE:
            return "update_available";
        default:
            return "";
    }
}

void sett::load_settings()
{
    using namespace fs;
    if( lock == NULL )
    {
        lock = xSemaphoreCreateMutex();
    }
    if( !exists("/settings.json") )
    {
        THROWE(SETTING_ERR_NO_FILE, "Cannot load settings, settings.json doesn't exist");
    }

    struct stat s = stat("/settings.json");
    char* buffer = new char[s.st_size];

    file f = open("/settings.json", "r");
    f.read(buffer, 1, s.st_size);

    settings = cJSON_Parse(buffer);
    delete[] buffer;

    if(settings == NULL )
    {
        THROWE(SETTING_ERR_PARSE, "Unable to parse settings.json (%s)", cJSON_GetErrorPtr());
    }
}

void sett::save_settings()
{
    using namespace fs;
    char* json_str = cJSON_Print(settings);
    if( json_str == NULL )
    {
        THROWE(SETTING_ERR_NULL, "Failed to print settings json");
    }

    file f = open("/settings.json", "w");
    f.write(json_str, 1, strlen(json_str));
}

std::string sett::read_str(sett::Key key)
{
    cJSON* object = cJSON_GetObjectItem(settings, get_key(key));
    if( !object )
    {
        THROWE(SETTING_ERR_INVALID_KEY, "Invalid key %s", get_key(key));
    }

    if( !cJSON_IsString(object) )
    {
        THROWE(SETTING_ERR_WRONG_TYPE, "%s is not a string", get_key(key));
    }

    return std::string(object->valuestring);
}

bool sett::read_bool(sett::Key key)
{
    cJSON* object = cJSON_GetObjectItem(settings, get_key(key));
    if( !object )
    {
        THROWE(SETTING_ERR_INVALID_KEY, "Invalid key %s", get_key(key));
    }

    if( !cJSON_IsBool(object) )
    {
        THROWE(SETTING_ERR_WRONG_TYPE, "%s is not a bool", get_key(key));
    }

    if( cJSON_IsTrue(object) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

void sett::write(sett::Key key, const char* value)
{
    ESP_LOGI(TAG, "Writing %s to %s", value, get_key(key) );

    cJSON* object = cJSON_GetObjectItem(settings, get_key(key));
    if( !cJSON_IsString(object) )
    {
        THROWE(SETTING_ERR_WRONG_TYPE, "%s is not a string", get_key(key));
    }

    if( cJSON_SetValuestring(object, value) == NULL )
    {

        THROWE(ESP_ERR_NO_MEM, "Error allocating space for %s", value);
    }

    save_settings();
}

void sett::write(sett::Key key, bool value)
{
    ESP_LOGI(TAG, "Writing %d to %s", value, get_key(key) );

    cJSON* object = cJSON_GetObjectItem(settings, get_key(key));
    if( !cJSON_IsBool(object) )
    {
        THROWE(SETTING_ERR_WRONG_TYPE, "%s is not a bool", get_key(key));
    }

    cJSON_DeleteItemFromObject(settings, get_key(key));
    if( value )
    {
        cJSON_AddTrueToObject(settings, get_key(key));
    }
    else
    {
        cJSON_AddFalseToObject(settings, get_key(key));
    }

    save_settings();
}