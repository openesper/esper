#include "settings.h"
#include "error.h"
#include "events.h"
#include "filesystem.h"
#include "string.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "SETTINGS";


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
        case BLOCKING:
            return "blocking";
        case UPDATE_AVAILABLE:
            return "update_available";
        case UPDATE_STATUS:
            return "update_status";
        default:
            return "";
    }
}

esp_err_t read_setting(Setting key, char* buffer)
{
    cJSON* json = read_settings_json();
    if( json == NULL)
    {
        log_error(ESP_ERR_NOT_FOUND, "read_settings_json()", __func__, __FILE__);
        return ESP_FAIL;
    }

    cJSON* object = cJSON_GetObjectItem(json, get_key(key));
    if( !cJSON_IsString(object) || object->valuestring == NULL )
    {
        cJSON_Delete(json);
        return FS_ERR_INVALID_KEY;
    }

    strcpy(buffer, object->valuestring);
    cJSON_Delete(json);
    return ESP_OK;
}

esp_err_t read_setting(Setting key, bool* value)
{
    cJSON* json = read_settings_json();
    if( json == NULL)
    {
        log_error(ESP_ERR_NOT_FOUND, "read_settings_json()", __func__, __FILE__);
        return ESP_FAIL;
    }

    cJSON* object = cJSON_GetObjectItem(json, get_key(key));
    if( !cJSON_IsBool(object) )
    {
        cJSON_Delete(json);
        return FS_ERR_INVALID_KEY;
    }

    if( cJSON_IsTrue(object) )
    {
        *value = true;
    }
    else
    {
        *value = false;
    }

    cJSON_Delete(json);
    return ESP_OK;
}

esp_err_t write_setting(Setting key, char* buffer)
{
    if( !buffer )
    {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON* json = read_settings_json();
    if( json == NULL)
    {
        return ESP_FAIL;
    }

    cJSON* object = cJSON_GetObjectItem(json, get_key(key));
    if( !cJSON_IsString(object) || object->valuestring == NULL )
    {
        cJSON_Delete(json);
        return FS_ERR_INVALID_KEY;
    }

    if( cJSON_SetValuestring(object, buffer) == NULL )
    {
        return ESP_ERR_INVALID_ARG;
    }

    if( write_settings_json(json) != ESP_OK )
    {
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    cJSON_Delete(json);
    return ESP_OK;
}

esp_err_t write_settings_json(cJSON* json)
{
    FILE* settings = open_file("/app/settings.json", "w");
    if( !settings )
    {
        log_error(ESP_FAIL, "open_file(\"/app/settings.json\", \"w\")", __func__, __FILE__);
        return ESP_FAIL;
    }

    char* json_str = cJSON_Print(json); 
    if( fwrite(json_str, 1, strlen(json_str), settings) < 1 )
    {
        log_error(ESP_FAIL, "fwrite(json_str, 1, strlen(json_str)+1, settings)", __func__, __FILE__);
        fclose(settings);
        return ESP_FAIL;
    }
    fclose(settings);
    
    return ESP_OK;
}

cJSON* read_settings_json()
{
    char buffer[1000+1];
    FILE* settings = open_file("/app/settings.json", "r");
    if( settings == NULL )
    {
        log_error(ESP_ERR_NOT_FOUND, "open_file(\"/app/settings.json\", \"r\")", __func__, __FILE__);
        return NULL;
    }

    int bytes_read;
    if( (bytes_read = fread(buffer, 1, 1000, settings)) < 1 )
        return NULL;
    
    buffer[bytes_read] = '\0';
    fclose(settings);

    return cJSON_Parse(buffer);
}
