#ifndef PUT_HANDLERS_H
#define PUT_HANDLERS_H

#include "esp_system.h"
#include "esp_http_server.h"

esp_err_t register_put_handlers(httpd_handle_t server);

#endif