#ifndef SETTINGS_H
#define SETTINGS_H

#include "esp_system.h"
#include <string>

// class Settings {
//         fs::file file_handle;
//         void* json;
//     public:
//         Settings();
//         ~Settings();
// };

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


#endif