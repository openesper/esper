#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "esp_system.h"
#include "sys/stat.h"

#include <string>

#define MAX_FILENAME_LENGTH CONFIG_LITTLEFS_OBJ_NAME_LEN

namespace fs
{
    class file{
        public:
            FILE* handle;
            std::string fpath;
            
            file(std::string path, const char* mode);
            ~file();
            size_t read(void* buffer, size_t size, size_t count);
            size_t write(const void* buffer, size_t size, size_t count);
    };

    file open(std::string path, const char* mode);
    bool exists(std::string path);
    struct stat stat(std::string path);
    void unlink(std::string path);
    void rename(std::string before, std::string after);
}

void reset_fs();
void init_fs();

#endif