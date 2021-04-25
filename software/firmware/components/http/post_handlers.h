#ifndef POST_HANDLERS_H
#define POST_HANDLERS_H

#include "esp_system.h"
#include "esp_http_server.h"

esp_err_t submitauth_handler(httpd_req_t *req);
esp_err_t scan_handler(httpd_req_t *req);
esp_err_t finish_handler(httpd_req_t *req);

#endif