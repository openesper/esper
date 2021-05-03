#include "gpio.h"
#include "error.h"
#include "events.h"
#include "dns.h"
#include "settings.h"
#include "ota.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "nvs_flash.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "GPIO";

#define LED_SPEED_MODE LEDC_LOW_SPEED_MODE      // LED PWM speed mode
#define MAX_DUTY_CYCLE 1023                     // Duty cycle, picked 1023 so 0-255 values can be used for color 
#define FADE_TIME 1000                          // Fade time of 1 second

#define RED_LED_CHANNEL LEDC_CHANNEL_0          // Red LED Channel
#define GREEN_LED_CHANNEL LEDC_CHANNEL_1        // Green LED Channel
#define BLUE_LED_CHANNEL LEDC_CHANNEL_2         // Blue LED Channel

static gpio_num_t button = GPIO_NUM_NC;         // Button GPIO
static gpio_num_t red_led = GPIO_NUM_NC;        // Red LED GPIO
static gpio_num_t green_led = GPIO_NUM_NC;      // Green LED GPIO
static gpio_num_t blue_led = GPIO_NUM_NC;       // Blue LED GPIO

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

        ESP_LOGD(TAG, "Reset Pin intr, val: %d\n", gpio_get_level(button));

        if(gpio_get_level(button) == 0) // Button is pressed down
        {
            press_timestamp = esp_log_timestamp();
        }
        else    // Button is released
        {
            length = esp_log_timestamp() - press_timestamp;
            if( length > 8000 )
            {
                ESP_LOGW(TAG, "Rolling back");
                rollback();
                esp_restart();
            }
            else
            {
                bool blocking = !sett::read_bool(sett::BLOCK);
                sett::write(sett::BLOCK, blocking);
            }
        }
    }
}

esp_err_t set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{

    ledc_set_duty(LED_SPEED_MODE, RED_LED_CHANNEL, red*4);
    ledc_update_duty(LED_SPEED_MODE, RED_LED_CHANNEL);

    ledc_set_duty(LED_SPEED_MODE, GREEN_LED_CHANNEL, green*4);
    ledc_update_duty(LED_SPEED_MODE, GREEN_LED_CHANNEL);

    ledc_set_duty(LED_SPEED_MODE, BLUE_LED_CHANNEL, blue*4);
    ledc_update_duty(LED_SPEED_MODE, BLUE_LED_CHANNEL);

    return ESP_OK;
}

TaskHandle_t getLEDTaskHandle()
{
    return led_task_handle;
}

static void led_task(void* args)
{
    uint32_t state = 0;
    while(1)
    {
        xTaskNotifyWait(0, 0, &state, portMAX_DELAY);
        
        if( check_bit(ERROR_BIT) )
        {
            set_rgb(RED);
        }
        else if( check_bit(INITIALIZING_BIT) )
        {
            set_rgb(PURPLE);
        }
        else if( check_bit(BLOCKED_QUERY_BIT) )
        {
            set_rgb(OFF);
            vTaskDelay( 50/portTICK_PERIOD_MS );

            clear_bit(BLOCKED_QUERY_BIT);
        }
        else if( sett::read_bool(sett::BLOCK) )
        {
            set_rgb(BLUE);
        }
        else 
        {
            set_rgb(WEAK_BLUE);
        }
    }
}

static esp_err_t init_button()
{
    gpio_pad_select_gpio(button);
    ATTEMPT(gpio_set_direction(button, GPIO_MODE_INPUT))
    ATTEMPT(gpio_set_pull_mode(button, GPIO_PULLUP_ONLY))
    ATTEMPT(gpio_set_intr_type(button, GPIO_INTR_ANYEDGE))
    ATTEMPT(gpio_install_isr_service(0))
    ATTEMPT(gpio_isr_handler_add(button, button_state_change, NULL))
    
    return ESP_OK;
}

static esp_err_t init_leds()
{
    // Set up LED PWM timer configs 
    ledc_timer_config_t led_timer_config = {
        .speed_mode = LED_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ATTEMPT(ledc_timer_config(&led_timer_config))
    
    // Configure Blue LED
    ledc_channel_config_t led_channel_config_blue = {
        .gpio_num = blue_led,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = BLUE_LED_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ATTEMPT(ledc_channel_config(&led_channel_config_blue))
    
    // Configure Red LED
    ledc_channel_config_t led_channel_config_red = {
        .gpio_num = red_led,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = RED_LED_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ATTEMPT(ledc_channel_config(&led_channel_config_red))
    

    // Configure Green LED
    ledc_channel_config_t led_channel_config_green = {
        .gpio_num = green_led,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = GREEN_LED_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ATTEMPT(ledc_channel_config(&led_channel_config_green))
    
    set_rgb(INITIALIZING);
    return ESP_OK;
}

void init_gpio()
{
    if( check_bit(GPIO_ENABLED_BIT) )
    {
        ESP_LOGI(TAG, "Initializing GPIO...");

        
        nvs_handle nvs;
        esp_err_t err;
        if ( (err = nvs_open("storage", NVS_READONLY, &nvs)) != ESP_OK )
        {
            THROWE(err, "Failed to open nvs")
        }

        uint8_t btn,r,g,b;
        err = nvs_get_u8(nvs, "button", &btn);
        err |= nvs_get_u8(nvs, "red_led", &r);
        err |= nvs_get_u8(nvs, "green_led", &g);
        err |= nvs_get_u8(nvs, "blue_led", &b);
        if( err != ESP_OK )
        {
            THROWE(GPIO_ERR_INIT, "Failed to load gpio settings from nvs");
        }

        button    = static_cast<gpio_num_t>(btn);
        red_led   = static_cast<gpio_num_t>(r);
        green_led = static_cast<gpio_num_t>(g);
        blue_led  = static_cast<gpio_num_t>(b);

        if( init_leds() != ESP_OK || init_button() != ESP_OK )
        {
            THROWE(GPIO_ERR_INIT, "Failed to initialize gpio")
        }

        if( xTaskCreatePinnedToCore(led_task, "led_task", 2000, NULL, 2, &led_task_handle, tskNO_AFFINITY) == pdFAIL )
        {
            THROWE(GPIO_ERR_INIT, "Failed to start led_task")
        }

        if( xTaskCreatePinnedToCore(button_task, "button_task", 3000, NULL, 2, &button_task_handle, tskNO_AFFINITY) == pdFAIL )
        {
            THROWE(GPIO_ERR_INIT, "Failed to start button_task")
        }
    }
}