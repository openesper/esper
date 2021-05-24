#ifndef DELETE_HANDLERS_H
#define DELETE_HANDLERS_H

#include "esp_system.h"
#include "esp_http_server.h"

esp_err_t register_delete_handlers(httpd_handle_t server);

#endif