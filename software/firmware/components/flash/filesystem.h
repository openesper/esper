#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "esp_system.h"
#include "cJSON.h"

cJSON* get_settings_json();
esp_err_t init_filesystem();

#endif