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

using namespace setting;
static const char* get_key(Key s)
{
    switch( s )
    {
        case IP:
            return "ip";
        case NETMASK:
            return "netmask";
        case GATEWAY:
            return "gateway";
        case SSID:
            return "ssid";
        case PASSWORD:
            return "password";
        case UPDATE_SRV:
            return "update_srv";
        case HOSTNAME:
            return "url";
        case DNS_SRV:
            return "dns_srv";
        case VERSION:
            return "version";
        case BLOCK:
            return "blocking";
        case UPDATE_AVAILABLE:
            return "update_available";
        default:
            return "";
    }
}

void setting::load_settings()
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

void setting::save_settings()
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

std::string setting::read_str(Key key)
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

bool setting::read_bool(Key key)
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

void setting::write(Key key, const char* value)
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

void setting::write(Key key, bool value)
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

    if( key == BLOCK)
    {
        value ? set_bit(BLOCKING_BIT):clear_bit(BLOCKING_BIT);
    }

    save_settings();
}