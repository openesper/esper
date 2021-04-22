#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H

#include <esp_http_server.h>

/**
  * @brief Setup captive portal handlers for provisioning server
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t configure_captive_portal_handlers(httpd_handle_t server);

/**
  * @brief Teardown captive portal handlers used for provisioning
  *
  * @return
  *    - ESP_OK Success
  *    - ESP_FAIL unable to get info from flash
  */
esp_err_t teardown_captive_portal_handlers(httpd_handle_t server);

#endif