#ifndef OTA_H
#define OTA_H

#include <esp_system.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// /**
//   * @brief Mark image as invalid and rollback to previous version 
//   *
//   * @return
//   *    - ESP_OK Success
//   *    - ESP_FAIL Failure
//   */
// esp_err_t rollback_ota();

// /**
//   * @brief Start OTA task
//   *
//   * @return
//   *    - ESP_OK Success
//   *    - ESP_FAIL Failure
//   */
// esp_err_t start_ota();

// /**
//   * @brief Get status of OTA task
//   *
//   * @return eTaskState - Status of OTA task
//   */
// eTaskState get_ota_task_status();

// /**
//   * @brief Get OTA status string, contains results of last attempt at OTA
//   *
//   * @return pointer to static buffer
//   */
// char* get_ota_status_string();

// /**
//   * @brief Get status of previous check for update
//   *
//   * @return pointer to static buffer
//   */
// char* get_update_check_status_string();

// /**
//   * @brief Start task that checks for updates
//   *
//   * @return
//   *    - ESP_OK Success
//   *    - ESP_FAIL Failure
//   */
// esp_err_t start_update_checking_task();

// /**
//   * @brief Tell update checking task to check for another update
//   *
//   * @return
//   *    - ESP_OK Success
//   *    - ESP_FAIL Failure
//   */
// void check_for_update();

#endif