#ifndef SETTINGS_H
#define SETTINGS_H

#include "esp_system.h"
#include "cJSON.h"

enum Setting {
    IP = 0,
    NETMASK,
    GATEWAY,
    SSID,
    PASSWORD,
    UPDATE_SRV,
    HOSTNAME,
    DNS_SRV,
    VERSION,
    BLOCK,
    UPDATE_AVAILABLE,
    UPDATE_STATUS
};

esp_err_t write_setting(Setting key, const char* value);
esp_err_t write_setting(Setting key, bool value);
esp_err_t read_setting(Setting key, char* value);
esp_err_t read_setting(Setting key, bool* value);
bool      read_setting(Setting key);
// cJSON* read_settings_json();
esp_err_t load_settings();
esp_err_t write_settings_json(cJSON* json);

#endif