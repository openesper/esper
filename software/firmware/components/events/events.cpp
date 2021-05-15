#include "events.h"
#include "error.h"
#include "freertos/event_groups.h"

#ifdef CONFIG_LOCAL_LOG_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#endif
#include "esp_log.h"
static const char *TAG = "EVENT";

EventGroupHandle_t event_group;
const int ALL_BITS = 0x00FFFFFFUL;
const int GPIO_ENABLED_BIT = BIT1;
const int ETH_ENABLED_BIT = BIT2;
const int ETH_INITIALIZED_BIT = BIT3;
const int ETH_CONNECTED_BIT = BIT4;
const int ETH_GOT_IP_BIT = BIT5;
const int WIFI_ENABLED_BIT = BIT6;
const int WIFI_INITIALIZED_BIT = BIT7;
const int WIFI_CONNECTED_BIT = BIT8;
const int WIFI_GOT_IP_BIT = BIT9;

const int INITIALIZING_BIT = BIT12;
const int ERROR_BIT = BIT13;
const int BLOCKING_BIT = BIT15;
const int BLOCKED_QUERY_BIT = BIT16;


void init_event_group()
{
    event_group = xEventGroupCreate();;
    if( event_group == NULL )
    {
        THROWE(EVENT_ERR_INIT, "Error creating event group")
    }
}

void set_bit(int bit)
{
    ESP_LOGD(TAG, "Setting %d", bit);
    xEventGroupSetBits(event_group, bit);
}

void clear_bit(int bit)
{
    ESP_LOGD(TAG, "Clearing %d", bit);
    xEventGroupClearBits(event_group, bit);
}

bool check_bit(int bit)
{
    return (xEventGroupGetBits(event_group) & bit);
}

void wait_for(int bit, TickType_t xTicksToWait)
{
    ESP_LOGD(TAG, "Waiting for %X", bit);
    uint32_t bits_before = get_bits();
    uint32_t bits_after = xEventGroupWaitBits(event_group, bit, pdFALSE, pdFALSE, xTicksToWait);
    if( bits_before == bits_after ) // bits will be equal if timeout expired
    {
        ESP_LOGD(TAG, "Timeout while waiting for bits %X", bit);
        // THROWE(ESP_ERR_TIMEOUT, "Timeout while waiting for bits %X", bit)
    }
}

void toggle_bit(int bit)
{
    ESP_LOGD(TAG, "Toggling %d", bit);
    check_bit(bit) ? clear_bit(bit) : set_bit(bit);
}

uint32_t get_bits()
{
    return xEventGroupGetBits(event_group);
}