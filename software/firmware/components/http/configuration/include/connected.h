#ifndef CONNECTED_H
#define CONNECTED_H

#include <esp_http_server.h>

esp_err_t configure_connected_handler(httpd_handle_t server);
esp_err_t teardown_connected_handler(httpd_handle_t server);

#endif