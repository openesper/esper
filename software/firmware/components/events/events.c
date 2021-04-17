#include "events.h"
#include "error.h"
#include "gpio.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "EVENT";

EventGroupHandle_t event_group;
const int SCAN_FINISHED_BIT = BIT0;
const int CONNECTED_BIT = BIT1;
const int DISCONNECTED_BIT = BIT2;
const int PROVISIONING_BIT = BIT3;
const int PROVISIONED_BIT = BIT4;
const int STATIC_IP_BIT = BIT5;
const int BLOCKING_BIT = BIT6;
const int INITIALIZING_BIT = BIT7;
const int ERROR_BIT = BIT8;
const int OTA_BIT = BIT9;
const int UPDATE_AVAILABLE_BIT = BIT10;
const int ETHERNET_CONNECTED_BIT = BIT11;
const int WIFI_CONNECTED_BIT = BIT12;
const int BLOCKED_QUERY_BIT = BIT13;

esp_err_t init_event_group()
{
    event_group = xEventGroupCreate();
    if( event_group == NULL )
    {
        ESP_LOGE(TAG, "Failed to start event group!");
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGI(TAG, "Event Group Started");
        return ESP_OK;
    }
}

esp_err_t set_bit(int bit)
{
    if( xEventGroupSetBits(event_group, bit) == pdFALSE )
    {
        ESP_LOGD(TAG, "Failed to set bit %X", bit);
    }

    // notify LED task of state change
    TaskHandle_t led_task = getLEDTaskHandle();
    xTaskNotify(led_task, xEventGroupGetBits(event_group), eSetValueWithOverwrite);
    ESP_LOGD(TAG, "Current state %X", xEventGroupGetBits(event_group));

    return ESP_OK;
}

esp_err_t clear_bit(int bit)
{
    if( xEventGroupClearBits(event_group, bit) == pdFALSE )
    {
        ESP_LOGD(TAG, "Failed to clear bit %X", bit);
    }

    // notify LED task of state change
    TaskHandle_t led_task = getLEDTaskHandle();
    xTaskNotify(led_task, xEventGroupGetBits(event_group), eSetValueWithOverwrite);
    ESP_LOGD(TAG, "Current state %X", xEventGroupGetBits(event_group));
    
    return ESP_OK;
}

bool check_bit(int bit)
{
    return (xEventGroupGetBits(event_group) & bit);
}

esp_err_t wait_for(int bit, TickType_t xTicksToWait)
{
    xEventGroupWaitBits(event_group, bit, pdFALSE, pdFALSE, xTicksToWait);
    return ESP_OK;
}

esp_err_t toggle_bit(int bit)
{
    if( check_bit(bit) )
    {
        ERROR_CHECK(clear_bit(bit))
    }
    else
    {
        ERROR_CHECK(set_bit(bit))
    }

    return ESP_OK;
}

uint32_t get_bits()
{
    return xEventGroupGetBits(event_group);
}