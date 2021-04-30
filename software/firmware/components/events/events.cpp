#include "events.h"
#include "error.h"
#include "gpio.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "EVENT";

EventGroupHandle_t event_group;
const int GPIO_ENABLED_BIT = BIT1;
const int ETH_ENABLED_BIT = BIT2;
const int ETH_INITIALIZED_BIT = BIT3;
const int ETH_CONNECTED_BIT = BIT4;
const int ETH_GOT_IP_BIT = BIT5;
const int WIFI_ENABLED_BIT = BIT6;
const int WIFI_INITIALIZED_BIT = BIT7;
const int WIFI_CONNECTED_BIT = BIT8;
const int WIFI_GOT_IP_BIT = BIT9;

const int PROVISIONING_BIT = BIT10;
const int INITIALIZING_BIT = BIT12;
const int ERROR_BIT = BIT13;
const int OTA_BIT = BIT14;
const int UPDATE_AVAILABLE_BIT = BIT15;
const int BLOCKED_QUERY_BIT = BIT16;


esp_err_t init_event_group()
{
    event_group = xEventGroupCreate();
    if( event_group == NULL )
    {
        log_error(EVENT_ERR_INIT, "xEventGroupCreate()", __func__, __FILE__);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Event Group Started");
    return ESP_OK;
}

esp_err_t set_bit(int bit)
{
    if( xEventGroupSetBits(event_group, bit) == pdFALSE )
    {
        ESP_LOGD(TAG, "Failed to set bit %X", bit);
    }

    return ESP_OK;
}

esp_err_t clear_bit(int bit)
{
    if( xEventGroupClearBits(event_group, bit) == pdFALSE )
    {
        ESP_LOGD(TAG, "Failed to clear bit %X", bit);
    }
    
    return ESP_OK;
}

bool check_bit(int bit)
{
    return (xEventGroupGetBits(event_group) & bit);
}

esp_err_t wait_for(int bit, TickType_t xTicksToWait)
{
    uint32_t bits_before = get_bits();
    uint32_t bits_after = xEventGroupWaitBits(event_group, bit, pdFALSE, pdFALSE, xTicksToWait);
    if( bits_before == bits_after ) // bits will be equal if timeout expired
        return ESP_FAIL;
    return ESP_OK;
}

esp_err_t toggle_bit(int bit)
{
    if( check_bit(bit) )
    {
        ATTEMPT(clear_bit(bit))
    }
    else
    {
        ATTEMPT(set_bit(bit))
    }

    return ESP_OK;
}

uint32_t get_bits()
{
    return xEventGroupGetBits(event_group);
}