#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "esp_system.h"

#define MAX_FILENAME_LENGTH 64

FILE* open_file(const char* filename, const char* mode);
bool file_exists(const char* filename);
// int stat_file(const char* filename, struct stat* s);
esp_err_t link_file(const char* src, const char* dst);
esp_err_t delete_file(const char* filename);
esp_err_t rename_file(const char* before, const char* after);
esp_err_t init_filesystem();

#endif