#include "gpio.h"
#include "error.h"
#include "events.h"
#include "flash.h"
#include "dns.h"
#include "error.h"
#include "ota.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "GPIO";

#define LED_SPEED_MODE LEDC_LOW_SPEED_MODE      // LED PWM speed mode
#define MAX_DUTY_CYCLE 1023                     // Duty cycle, picked 1023 so 0-255 values can be used for color 
#define FADE_TIME 1000                          // Fade time of 1 second

#define RED_LED_CHANNEL LEDC_CHANNEL_0          // Red LED Channel
#define GREEN_LED_CHANNEL LEDC_CHANNEL_1        // Green LED Channel
#define BLUE_LED_CHANNEL LEDC_CHANNEL_2         // Blue LED Channel

static bool gpio_enabled = true;                // GPIO enabled status
static int button = -1;                         // Button GPIO
static int red_led = -1;                        // Red LED GPIO
static int green_led = -1;                      // Green LED GPIO
static int blue_led = -1;                       // Blue LED GPIO

static TaskHandle_t button_task_handle;         // Task handle for button listening task
static TaskHandle_t led_task_handle;            // Task handle for LED task

static void button_state_change(void* arg)      // Button interrupt handler 
{
    xTaskNotifyFromISR(button_task_handle, 0, eNoAction, NULL);
}

static void button_task(void* args)             // Task keeping track of time between button presses
{
    uint32_t press_timestamp = 0xFFFFFFFF;
    uint32_t length = 0;

    while(1)
    {
        xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);

        if( gpio_enabled )
        {
            ESP_LOGD(TAG, "Reset Pin intr, val: %d\n", gpio_get_level(button));

            if(gpio_get_level(button) == 0) // Button is pressed down
            {
                press_timestamp = esp_log_timestamp();
            }
            else    // Button is released
            {
                length = esp_log_timestamp() - press_timestamp;
                if( length > 5000 )
                {
                    ESP_LOGW(TAG, "Resetting device");
                    reset_device();
                    esp_restart();
                }
                else if( length > 10000 )
                {
                    ESP_LOGW(TAG, "Rolling back");
                    rollback_ota();
                    esp_restart();
                }
                else
                {
                    toggle_bit(BLOCKING_BIT);
                }
            }
        }
    }
}

static esp_err_t get_rgb(uint8_t* red, uint8_t* green, uint8_t* blue)
{
    if( gpio_enabled )
    {
        *red = ledc_get_duty(LED_SPEED_MODE, RED_LED_CHANNEL);
        *green = ledc_get_duty(LED_SPEED_MODE, GREEN_LED_CHANNEL);
        *blue = ledc_get_duty(LED_SPEED_MODE, BLUE_LED_CHANNEL);
    }

    return ESP_OK;
}

esp_err_t set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    if( gpio_enabled )
    {
        ledc_set_duty(LED_SPEED_MODE, RED_LED_CHANNEL, red*4);
        ledc_update_duty(LED_SPEED_MODE, RED_LED_CHANNEL);

        ledc_set_duty(LED_SPEED_MODE, GREEN_LED_CHANNEL, green*4);
        ledc_update_duty(LED_SPEED_MODE, GREEN_LED_CHANNEL);

        ledc_set_duty(LED_SPEED_MODE, BLUE_LED_CHANNEL, blue*4);
        ledc_update_duty(LED_SPEED_MODE, BLUE_LED_CHANNEL);
    }

    return ESP_OK;
}

TaskHandle_t getLEDTaskHandle()
{
    return led_task_handle;
}

static void led_task(void* args)
{
    ESP_LOGI(TAG, "Listening for events");

    uint32_t state = 0;
    while(1)
    {
        xTaskNotifyWait(0, 0, &state, portMAX_DELAY);
        
        if( gpio_enabled )
        {
            if( check_bit(ERROR_BIT) )
            {
                set_rgb(RED);
            }
            else if( check_bit(PROVISIONING_BIT) )
            {
                set_rgb(GREEN);
            }
            else if( check_bit(INITIALIZING_BIT) )
            {
                set_rgb(PURPLE);
            }
            else if( check_bit(BLOCKED_QUERY_BIT) )
            {
                // uint8_t red, green, blue;
                // get_rgb(&red, &green, &blue);

                set_rgb(OFF);
                vTaskDelay( 50/portTICK_PERIOD_MS );

                // set_rgb(red, green, blue);
                clear_bit(BLOCKED_QUERY_BIT);
            }
            else if( check_bit(BLOCKING_BIT) )
            {
                set_rgb(BLUE);
            }
            else 
            {
                set_rgb(WEAK_BLUE);
            }
        }
    }
}

esp_err_t initialize_gpio()
{
    xTaskCreatePinnedToCore(button_task, "button_task", 3000, NULL, 2, &button_task_handle, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(led_task, "led_task", 2000, NULL, 2, &led_task_handle, tskNO_AFFINITY);

    ERROR_CHECK(get_gpio_config(&gpio_enabled, &button, &red_led, &green_led, &blue_led))
    if( !gpio_enabled )
    {
        ESP_LOGI(TAG, "GPIO Disabled");
        return ESP_OK;
    }
    // Set button settings and assign interrupt handler
    gpio_pad_select_gpio(button);
    gpio_set_direction(button, GPIO_MODE_DEF_INPUT);
    gpio_set_pull_mode(button, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(button, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(button, button_state_change, NULL);

    // Set up LED PWM timer configs 
    ledc_timer_config_t led_timer_config = {
        .speed_mode = LED_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .duty_resolution = LEDC_TIMER_10_BIT,
    };
    ledc_timer_config(&led_timer_config);

    // Set up LED PWM channel configs 
    ledc_channel_config_t led_channel_config_blue = {
        .gpio_num = blue_led,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = BLUE_LED_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
    };
    ledc_channel_config(&led_channel_config_blue);

    ledc_channel_config_t led_channel_config_red = {
        .gpio_num = red_led,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = RED_LED_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
    };
    ledc_channel_config(&led_channel_config_red);

    ledc_channel_config_t led_channel_config_green = {
        .gpio_num = green_led,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = GREEN_LED_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
    };
    ledc_channel_config(&led_channel_config_green);
    // ledc_fade_func_install(0);
    set_rgb(0,0,0);
    
    ESP_LOGI(TAG, "GPIO Initialized");
    return ESP_OK;
}
