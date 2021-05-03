#ifndef OTA_H
#define OTA_H

#include <esp_system.h>

void rollback();
void cancel_rollback();
void check_for_update();
void update_firmware();
esp_err_t init_ota();

#endif