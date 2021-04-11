#include "ota.h"
#include "error.h"
#include "events.h"
#include "gpio.h"
#include "url.h"
#include "flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "string.h"
#include "errno.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "OTA";

#define BUFFSIZE 1024                                   // Size of buffer containing OTA data
#define UPDATE_CHECK_INTERVAL_S 3600                    // Interval for checking for udpates

static char update_check_status_string[100] = "\0";     // String containing status of previous update check
static TaskHandle_t update_check_handle = NULL;         // Handle for update checking task

static char ota_status_string[100] = "";                // String containing status of previous ota
static TaskHandle_t ota_task_handle = NULL;             // Handle for OTA task
static char ota_write_data[BUFFSIZE + 1] = { 0 };       // Buffer for ota data


esp_err_t rollback_ota()
{
    esp_ota_mark_app_invalid_rollback_and_reboot();
    return ESP_OK;
}

char* get_update_check_status_string()
{
    return update_check_status_string;
}

static void check_for_update_task(void *pvParameter)
{
    while(1)
    {
        xTaskNotifyWait(0, 0xFFFFFFFF, NULL, portMAX_DELAY);
        clear_bit(UPDATE_AVAILABLE_BIT);
        
        // Get update server from flash
        char updatesrv[MAX_URL_LENGTH];
        get_update_url(updatesrv);
        esp_http_client_config_t config = {
            .url = updatesrv,
            .timeout_ms = 2000,
        };
        ESP_LOGI(TAG, "Checking %s for update...", updatesrv);

        // Initialize HTTP connection
        esp_http_client_handle_t client = esp_http_client_init(&config);
        if ( !client ) {
            ESP_LOGE(TAG, "Failed to initialise HTTP connection");
            strcpy(update_check_status_string, "Failed to initialise HTTP connection");
            continue;
        }

        // Open HTTP connection
        esp_err_t err = esp_http_client_open(client, 0);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
            esp_http_client_cleanup(client);
            strcpy(update_check_status_string, "Failed to open HTTP connection");
            continue;
        }
        esp_http_client_fetch_headers(client);

        // Read chunk of response
        int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
        if (data_read <= 0) 
        {
            ESP_LOGE(TAG, "Error: no data read");
        } 
        else if (data_read > 0)
        {
            esp_app_desc_t new_app_info;
            if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) 
            {
                // check current version with downloading
                memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));

                // Check to verify data is an esp app
                if ( new_app_info.magic_word != 0xABCD5432)
                {
                    ESP_LOGI(TAG, "New firmware has invalid magic word (%X)", new_app_info.magic_word);
                }
                else
                {
                    ESP_LOGI(TAG, "New firmware version: %.8s", new_app_info.version);

                    // Get data about running app
                    esp_app_desc_t running_app_info;
                    const esp_partition_t *running = esp_ota_get_running_partition();
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Running firmware version: %.8s", running_app_info.version);
                    }

                    // Set update status
                    if ( strncmp(running_app_info.version, new_app_info.version, 8) < 0 )
                    {
                        ESP_LOGI(TAG, "Update Available!");
                        set_bit(UPDATE_AVAILABLE_BIT);
                    }
                    else
                    {
                        clear_bit(UPDATE_AVAILABLE_BIT);
                    }

                    strcpy(update_check_status_string, "");
                }
            } 
            else 
            {
                ESP_LOGE(TAG, "received package too small");
            }
        }
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
    }
}

void check_for_update()
{
    xTaskNotify(update_check_handle, 1, eSetValueWithoutOverwrite);
}

char* get_ota_status_string()
{
    return ota_status_string;
}

static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static void ota_task(void *pvParameter)
{
    // Most of this code if from the OTA example app
    while(1)
    {
        vTaskSuspend(NULL);
        // xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
        esp_err_t err;
        /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
        esp_ota_handle_t update_handle = 0 ;
        const esp_partition_t *update_partition = NULL;

        ESP_LOGI(TAG, "Starting OTA");

        const esp_partition_t *configured = esp_ota_get_boot_partition();
        const esp_partition_t *running = esp_ota_get_running_partition();

        if (configured != running) {
            ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                    configured->address, running->address);
            ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
        }
        ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
                running->type, running->subtype, running->address);

        char updatesrv[MAX_URL_LENGTH];
        get_update_url(updatesrv);
        esp_http_client_config_t config = {
            .url = updatesrv,
            .timeout_ms = 2000,
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (client == NULL) {
            ESP_LOGE(TAG, "Failed to initialise HTTP connection");
            strcpy(ota_status_string, "Failed to initialise HTTP connection");
            continue;
        }
        err = esp_http_client_open(client, 0);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
            strcpy(ota_status_string, "Failed to open HTTP connection");
            esp_http_client_cleanup(client);
            continue;
        }
        esp_http_client_fetch_headers(client);

        update_partition = esp_ota_get_next_update_partition(NULL);
        ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
                update_partition->subtype, update_partition->address);
        assert(update_partition != NULL);

        int binary_file_length = 0;
        /*deal with all receive packet*/
        bool image_header_was_checked = false;
        bool keep_going = true;
        while ( keep_going ) {
            vTaskDelay(1); // added to make FreeRTOS task watchdog happy
            int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
            if (data_read < 0) 
            {
                ESP_LOGE(TAG, "Error: SSL data read error");
                strcpy(ota_status_string, "Error: SSL data read error");
                http_cleanup(client);
                continue;
            } 
            else if (data_read > 0) 
            {
                if (image_header_was_checked == false) 
                {
                    esp_app_desc_t new_app_info;
                    if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) 
                    {
                        // check current version with downloading
                        memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                        ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

                        esp_app_desc_t running_app_info;
                        if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                            ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
                        }

                        const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
                        esp_app_desc_t invalid_app_info;
                        if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
                            ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
                        }

                        // check current version with last invalid partition
                        if (last_invalid_app != NULL) 
                        {
                            if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) 
                            {
                                ESP_LOGW(TAG, "New version is the same as invalid version.");
                                ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
                                ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
                                strcpy(ota_status_string, "New version is the same as previous invalid version.");
                                http_cleanup(client);
                                keep_going = false;
                                continue;
                            }
                        }

                        if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
                            ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
                            strcpy(ota_status_string, "Current running version is the same as a new. Update will not continue.");
                            http_cleanup(client);
                            keep_going = false;
                            continue;
                        }

                        image_header_was_checked = true;

                        err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
                        if (err != ESP_OK) 
                        {
                            ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                            strcpy(ota_status_string, "esp_ota_begin failed");
                            http_cleanup(client);
                            keep_going = false;
                            continue;
                        }
                        ESP_LOGI(TAG, "esp_ota_begin succeeded");
                    } 
                    else 
                    {
                        ESP_LOGE(TAG, "received package is not fit len");
                        strcpy(ota_status_string, "received package is not fit len");
                        http_cleanup(client);
                        keep_going = false;
                        continue;
                    }
                }
                err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
                if (err != ESP_OK) {
                    strcpy(ota_status_string, "esp_ota_write failed!");
                    http_cleanup(client);
                    keep_going = false;
                    continue;
                }
                binary_file_length += data_read;
                ESP_LOGD(TAG, "Written image length %d", binary_file_length);
            } 
            else if (data_read == 0) 
            {
            /*
                * As esp_http_client_read never returns negative error code, we rely on
                * `errno` to check for underlying transport connectivity closure if any
                */
                if (errno == ECONNRESET || errno == ENOTCONN) {
                    ESP_LOGE(TAG, "Connection closed, errno = %d", errno);
                    break;
                }
                if (esp_http_client_is_complete_data_received(client) == true) {
                    ESP_LOGI(TAG, "Connection closed");
                    break;
                }
            }
        }
        
        ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);
        
        if( keep_going )
        {
            if (esp_http_client_is_complete_data_received(client) != true) {
                ESP_LOGE(TAG, "Error in receiving complete file");
                strcpy(ota_status_string, "Error in receiving complete file");
                http_cleanup(client);
                continue;
            }

            err = esp_ota_end(update_handle);
            if (err != ESP_OK) {
                if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
                    ESP_LOGE(TAG, "Image validation failed, image is corrupted");
                }
                ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
                strcpy(ota_status_string, "esp_ota_end failed");
                http_cleanup(client);
                continue;
            }

            err = esp_ota_set_boot_partition(update_partition);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
                strcpy(ota_status_string, "esp_ota_set_boot_partition failed");
                http_cleanup(client);
                continue;
            }
            ESP_LOGI(TAG, "Prepare to restart system!");
            strcpy(ota_status_string, "");
        }
    }
}

eTaskState get_ota_task_status()
{
    eTaskState ota_state = eTaskGetState(ota_task_handle);
    return ota_state;
}

esp_err_t start_ota()
{
    // xTaskNotify(ota_task_handle, 1, eSetValueWithoutOverwrite);  
    vTaskResume(ota_task_handle);  
    return ESP_OK;
}

esp_err_t start_update_checking_task(){
    xTaskCreatePinnedToCore(&ota_task, "ota_task", 8192, NULL, 5, &ota_task_handle, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(&check_for_update_task, "update_check_task", 8192, NULL, 5, &update_check_handle, tskNO_AFFINITY);
    check_for_update();

    xTimerHandle updateCheckTimer = xTimerCreate("Check for update", pdMS_TO_TICKS(1000*60*60), pdTRUE, (void*)0, check_for_update);
    xTimerStart(updateCheckTimer, 0);

    return ESP_OK;
}