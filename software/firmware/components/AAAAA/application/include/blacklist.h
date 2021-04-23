#ifndef BLACKLIST_H
#define BLACKLIST_H

#include <esp_http_server.h>

esp_err_t setup_blacklist_handlers(httpd_handle_t server);
esp_err_t teardown_blacklist_handlers(httpd_handle_t server);

#endif

