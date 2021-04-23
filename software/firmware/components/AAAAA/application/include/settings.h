#ifndef SETTINGS_H
#define SETTINGS_H

#include <esp_http_server.h>

esp_err_t setup_settings_handlers(httpd_handle_t server);
esp_err_t teardown_settings_handlers(httpd_handle_t server);

#endif

