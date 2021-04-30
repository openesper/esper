#include "settings.h"
#include "error.h"
#include "events.h"
#include "filesystem.h"
#include "string.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "SETTINGS";

static cJSON* settings;

static cJSON* read_settings_json()
{
    cJSON* ret;
    char* buffer = NULL;
    FILE* settings_f = NULL;

    struct stat s;
    if( stat_file("/app/settings.json", &s) != ESP_OK )
    {
        log_error(ESP_ERR_NOT_FOUND, "open_file(\"/app/settings.json\", \"r\")", __func__, __FILE__);
        ret =  NULL;
        goto cleanup;
    }

    // This function gets called from esp event loop, so stack has to be kept small
    // buffer is kept in heap to prevent errors
    buffer = (char*)malloc(s.st_size+1); 
    settings_f = open_file("/app/settings.json", "r");
    if( settings_f == NULL )
    {
        log_error(ESP_ERR_NOT_FOUND, "open_file(\"/app/settings.json\", \"r\")", __func__, __FILE__);
        ret = NULL;
        goto cleanup;
    }

    int bytes_read;
    if( (bytes_read = fread(buffer, 1, s.st_size, settings_f)) < s.st_size )
    {
        log_error(ESP_ERR_NOT_FOUND, "fread(buffer, 1, s.st_size, settings))", __func__, __FILE__);
        ret = NULL;
        goto cleanup;
    }

    buffer[bytes_read] = '\0';
    ret = cJSON_Parse(buffer);

cleanup:
    if( settings_f )
        fclose(settings_f);
    if( buffer )
        free(buffer);

    return ret;
}

static const char* get_key(Setting s)
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
        case UPDATE_STATUS:
            return "update_status";
        default:
            return "";
    }
}

esp_err_t read_setting(Setting key, char* value)
{
    cJSON* object = cJSON_GetObjectItem(settings, get_key(key));
    if( !object )
    {
        return SETTING_ERR_INVALID_KEY;
    }

    if( !cJSON_IsString(object) )
    {
        return SETTING_ERR_WRONG_TYPE;
    }

    strcpy(value, object->valuestring);
    return ESP_OK;
}

esp_err_t read_setting(Setting key, bool* value)
{
    ESP_LOGI(TAG, "Reading %s", get_key(key));

    cJSON* object = cJSON_GetObjectItem(settings, get_key(key));
    if( !cJSON_IsBool(object) )
    {
        return SETTING_ERR_WRONG_TYPE;
    }

    if( cJSON_IsTrue(object) )
    {
        *value = true;
    }
    else
    {
        *value = false;
    }

    return ESP_OK;
}

bool read_setting(Setting key)
{
    ESP_LOGI(TAG, "Reading %s", get_key(key));

    cJSON* object = cJSON_GetObjectItem(settings, get_key(key));
    if( cJSON_IsTrue(object) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

esp_err_t write_setting(Setting key, const char* value)
{
    ESP_LOGI(TAG, "Writing %s to %s", value, get_key(key) );
    if( !value )
    {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON* object = cJSON_GetObjectItem(settings, get_key(key));
    if( !cJSON_IsString(object) )
    {
        return SETTING_ERR_WRONG_TYPE;
    }

    if( cJSON_SetValuestring(object, value) == NULL )
    {

        return ESP_ERR_INVALID_ARG;
    }

    if( write_settings_json(settings) != ESP_OK )
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t write_setting(Setting key, bool value)
{
    ESP_LOGI(TAG, "Writing %d to %s", value, get_key(key) );

    cJSON* object = cJSON_GetObjectItem(settings, get_key(key));
    if( !cJSON_IsBool(object) )
    {
        return SETTING_ERR_WRONG_TYPE;
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

    if( write_settings_json(settings) != ESP_OK )
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t write_settings_json(cJSON* json)
{
    FILE* settings_f = open_file("/app/settings.json", "w");
    if( !settings_f )
    {
        log_error(ESP_FAIL, "open_file(\"/app/settings.json\", \"w\")", __func__, __FILE__);
        return ESP_FAIL;
    }

    char* json_str = cJSON_Print(json); 
    if( fwrite(json_str, 1, strlen(json_str), settings_f) < 1 )
    {
        log_error(ESP_FAIL, "fwrite(json_str, 1, strlen(json_str)+1, settings)", __func__, __FILE__);
        fclose(settings_f);
        return ESP_FAIL;
    }
    fclose(settings_f);
    
    return ESP_OK;
}

esp_err_t load_settings()
{
    settings = read_settings_json();
    if( settings == NULL )
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}