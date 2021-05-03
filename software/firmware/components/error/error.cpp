#include "error.h"
#include "string.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
// static const char *TAG = "ERROR";


Err::Err(std::string msg_, int errcode_, const char* file_, const char* func_, int line_) throw()
: msg(msg_), errcode(errcode_), file(file_), func(func_), line(line_) {
    log_error(errcode, msg.c_str(), func, file, line);
}

const char* Err::what() const throw()
{
    return Err::msg.c_str();
}


void log_error(int err, const char* msg, const char* function, const char* file, int line)
{
    if( err < 1 )
    {
        ESP_LOGE(file, "%s at %d (%x, %s), %s", function, line, err, esp_err_to_name(err), msg);
    }
    else if( err < 0x100 )
    {
        ESP_LOGE(file, "%s at %d (%x, %s), %s", function, line, err, strerror(err), msg);
    }
    else if ( 0x100 < err && err < 0x200 )
    {
        ESP_LOGE(file, "%s at %d (%x, %s), %s", function, line, err, esp_err_to_name(err), msg);
    }
    else if ( err < 0x3000 )
    {
        ESP_LOGE(file, "%s at %d (%x, %s), %s", function, line, err, esp_err_to_name(err), msg);
    }
}