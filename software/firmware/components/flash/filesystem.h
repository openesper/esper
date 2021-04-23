#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "esp_system.h"
#include "cJSON.h"
#include "sys/stat.h"

#define MAX_FILENAME_LENGTH 64

FILE* open_file(const char* filename, const char* mode);
int stat_file(const char* filename, struct stat* s);
cJSON* get_settings_json();
esp_err_t init_filesystem();

#endif