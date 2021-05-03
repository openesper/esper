#ifndef SETTINGS_H
#define SETTINGS_H

#include "esp_system.h"
#include "cJSON.h"

#include <string>

namespace sett
{
    enum Key {
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
        UPDATE_AVAILABLE
    }; 

    void load_settings();
    void save_settings();
    std::string read_str(Key key);
    bool read_bool(Key key);
    void write(Key key, const char* value);
    void write(Key key, bool value);
}

//enum Setting {
//     IP = 0,
//     NETMASK,
//     GATEWAY,
//     SSID,
//     PASSWORD,
//     UPDATE_SRV,
//     HOSTNAME,
//     DNS_SRV,
//     VERSION,
//     BLOCK,
//     UPDATE_AVAILABLE
// };

// esp_err_t sett::write(Setting key, const char* value);
// esp_err_t sett::write(Setting key, bool value);
// esp_err_t sett::read(sett::Setting key, char* value);
// esp_err_t sett::read(sett::Setting key, bool* value);
// bool      sett::read(sett::Setting key);
// // cJSON* read_settings_json();
// esp_err_t load_settings();
// esp_err_t sett::writes_json(cJSON* json);


#endif