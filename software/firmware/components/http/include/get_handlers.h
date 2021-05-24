#ifndef GET_HANDLERS_H
#define GET_HANDLERS_H

#include "esp_system.h"
#include "esp_http_server.h"

esp_err_t register_get_handlers(httpd_handle_t server);

#endif