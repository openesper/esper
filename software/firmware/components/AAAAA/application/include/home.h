#ifndef HOME_H
#define HOME_H

#include <esp_http_server.h>

esp_err_t setup_homepage_handlers(httpd_handle_t server);
esp_err_t teardown_homepage_handlers(httpd_handle_t server);

#endif

