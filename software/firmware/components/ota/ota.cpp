#include "ota.h"
#include "error.h"
#include "events.h"
#include "settings.h"

#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "OTA";


#define BUFFSIZE 5120                                   // Size of buffer containing OTA data
#define UPDATE_CHECK_INTERVAL_S 3600                    // Interval for checking for udpates

static TaskHandle_t update_check_task_handle = NULL;    // Handle for update checking task

const static esp_partition_t *running;                  // Current partition


class ota {
        esp_http_client_handle_t client_handle;
        esp_ota_handle_t update_handle;
        const esp_partition_t *update_partition;
    public:
        ota(std::string server);
        ~ota();
        int get_content_len();
        void begin(const esp_partition_t *partition);
        int read(char* buf, int bufsize);
        void write(char* buffer, int bytes);
        void download();
        void end();
        bool data_received();
};

ota::ota(std::string server)
{
    esp_http_client_config_t config = {};         // Default http configuration
    config.host = server.c_str();
    config.path = "/OpenEsper.bin";
    config.transport_type = HTTP_TRANSPORT_OVER_TCP;
    config.timeout_ms = 2000;

    client_handle = esp_http_client_init(&config);
    if (client_handle == NULL) {
        THROW("Failed to initialise connection to %s", server.c_str())
    }

    if (esp_http_client_open(client_handle, 0) != ESP_OK) {
        THROW("Failed to open connection to %s", server.c_str())
    }
    esp_http_client_fetch_headers(client_handle);
}

ota::~ota()
{
    esp_ota_abort(update_handle);
    esp_http_client_close(client_handle);
    esp_http_client_cleanup(client_handle);
}

int ota::get_content_len()
{
    return esp_http_client_get_content_length(client_handle);
}

void ota::begin(const esp_partition_t *partition)
{
    update_handle = 0;
    update_partition = partition;

    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) 
    {
        THROW("esp_ota_begin failed (%s)", esp_err_to_name(err));
    }
}

int ota::read(char* buf, int bufsize)
{
    int data_read = esp_http_client_read(client_handle, buf, bufsize);
    if (data_read < 0) 
    {
        THROW("SSL data read error");
    }
    else if (data_read == 0)
    {
        if( errno == ECONNRESET) 
        {    
            THROW("Connection reset by peer");
        }
        else if( errno == ENOTCONN ) 
        {    
            THROW("Client not connected");
        }
    }
    return data_read;
}

void ota::write(char* buffer, int bytes)
{
    esp_err_t err = esp_ota_write( update_handle, (const void *)buffer, bytes);
    if (err != ESP_OK) {
        THROW("esp_ota_begin failed (%s)", esp_err_to_name(err));
    }
}

void ota::download()
{
    int counter = 0;
    int bytes_written = 0;
    int data_read = 0;
    static char ota_data[BUFFSIZE + 1] = { 0 }; 
    do {
        data_read = read(ota_data, BUFFSIZE);
        write(ota_data, data_read);

        counter += data_read;
        if( counter > 100000 )
        {
            bytes_written += counter;
            counter = 0;
            ESP_LOGI(TAG, "%d bytes written", bytes_written);
        }
    } while( data_read > 0);
    ESP_LOGI(TAG, "Total Write binary data length: %d", bytes_written);

}

bool ota::data_received()
{
    return esp_http_client_is_complete_data_received(client_handle);
}

void ota::end()
{
    esp_err_t err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        }
        THROW("esp_ota_end failed (%s)!", esp_err_to_name(err));
    }

    // change boot partition
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        THROW("esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
    }
}

void update_checking_task(void* parameters)
{
    while(1)
    {
        try{
            xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
            std::string updatesrv = sett::read_str(sett::UPDATE_SRV);
            ESP_LOGI(TAG, "Connecting to %s...", updatesrv.c_str());
            ota client(updatesrv);

            char buffer[512];
            int data_read = client.read(buffer, 512);
            if (data_read < sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
            {
                THROW("Header received from update server was too small");
            }

            // check current version with downloading
            esp_app_desc_t new_app_info;
            memcpy(&new_app_info, &buffer[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
            ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

            esp_app_desc_t running_app_info;
            esp_ota_get_partition_description(running, &running_app_info);
            ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);

            if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
                ESP_LOGW(TAG, "Current running version is the same as a new");
                sett::write(sett::UPDATE_AVAILABLE, false);
            }
            else if ( strncmp(running_app_info.version, new_app_info.version, 8) < 0 )
            {
                ESP_LOGI(TAG, "Update Available!");
                sett::write(sett::UPDATE_AVAILABLE, true);
            }
            else
            {
                sett::write(sett::UPDATE_AVAILABLE, false);
            }
        }catch(std::string e){
            continue;
        }
    }
}

void update_firmware()
{
    // Point to update server
    std::string updatesrv = sett::read_str(sett::UPDATE_SRV);
    ESP_LOGI(TAG, "Connecting to %s...", updatesrv.c_str());
    ota client(updatesrv);

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if( client.get_content_len() >= update_partition->size )
    {
        THROW("Update binary too large for partition");
    }

    ESP_LOGI(TAG, "Updating partition %s", update_partition->label);
    client.begin(update_partition);

    ESP_LOGI(TAG, "Beginning download...");
    client.download();

    if( client.data_received() )
    {
        ESP_LOGI(TAG, "Download complete");
    }
    else
    {
        THROW("Error in receiving complete file");
    }

    client.end();
    sett::write(sett::UPDATE_AVAILABLE, false);
    ESP_LOGI(TAG, "OTA Complete!");
}

void rollback()
{
    set_bit(ERROR_BIT);
    ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
    
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
    }

    ESP_LOGE(TAG, "No apps to revert to, rollback failed");
    vTaskDelay( 5000/portTICK_PERIOD_MS );
    esp_restart();
}

void cancel_rollback()
{
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }
}

void check_for_update()
{
    xTaskNotify(update_check_task_handle, 0, eNoAction);
}

static void updateCheckCallback(TimerHandle_t xTimer)
{
    check_for_update();
}

esp_err_t init_ota()
{
    running = esp_ota_get_running_partition();
    xTaskCreatePinnedToCore(&update_checking_task, "update_checking_task", 8192, NULL, 5, &update_check_task_handle, tskNO_AFFINITY);

    xTimerHandle updateCheckTimer = xTimerCreate("Check for update", pdMS_TO_TICKS(1000*10), pdTRUE, (void*)0, updateCheckCallback);
    xTimerStart(updateCheckTimer, 0);

    return ESP_OK;
}