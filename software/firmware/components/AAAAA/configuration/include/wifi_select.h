#ifndef WIFI_SELECT_H
#define WIFI_SELECT_H

#include <esp_http_server.h>

esp_err_t configure_wifi_select_handler(httpd_handle_t server);
esp_err_t teardown_wifi_select_handler(httpd_handle_t server);

#endif