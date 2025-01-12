#include "GraphicsDriver.h"
#define TAG "GraphicsDriver"

#include "TouchDriver.h"
#include "AwakeManager.h"
#include "ConfigManager.h"
#include "driver/ledc.h" 
#include "esp_task_wdt.h" 

#define min_task_delay 3

GraphicsDriver::GraphicsDriver() {
    // Constructor
}

void GraphicsDriver::init() {
    gc9a01_displayInit();
    lvglDisplayConfig();

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), LV_PART_MAIN);

    LvglMutex::init();

    // // Create LVGL task
    xTaskCreatePinnedToCore(lvgl_task, "Rendering Task", 128000, NULL, 4, NULL, 0);
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 5000,                // 5 second timeout
        .idle_core_mask = (1 << 0),        // Watch core 0
        .trigger_panic = true
    };
    ESP_ERROR_CHECK(esp_task_wdt_reconfigure(&twdt_config));
}

void GraphicsDriver::lvgl_task(void *arg) {
    ESP_LOGI(TAG, "Starting LVGL task");

    TickType_t last_wake_time = xTaskGetTickCount();
    const int target_frame_time_ms = 1000 / 30;

    // Debounce variables
    bool is_debouncing = false;
    TickType_t debounce_end_time = 0;

    while (1) {
        int screen_timeout_ms = ConfigManager::getConfigInt("General", "ScreenTimeout") * 1000;
        TickType_t start_time = xTaskGetTickCount();

        // Render within a LVGL mutex lock
        LvglMutex::lock();
        lv_timer_handler();
        LvglMutex::unlock();

        // How long did the frame take to render, then minus our tar
        TickType_t frame_time = xTaskGetTickCount() - start_time;
        int delay_time_ms = target_frame_time_ms - pdTICKS_TO_MS(frame_time);

        // Ensure a minimum delay of 1 ms to prevent CPU lockup
        if (delay_time_ms < min_task_delay) {
            delay_time_ms = min_task_delay;
        }

        vTaskDelay(pdMS_TO_TICKS(delay_time_ms));

        // Handle debounce after waking up from sleep
        if (is_debouncing) {
            if (xTaskGetTickCount() >= debounce_end_time) {
                is_debouncing = false; // End of debounce period
            } else {
                continue; // Skip the rest of the loop during debounce
            }
        }

        // Check inactivity time and put the device to sleep if needed
        if (lv_disp_get_inactive_time(NULL) > screen_timeout_ms) {
            ESP_LOGI(TAG, "SLEEPING DUE TO INACTIVITY");
            AwakeManager::sleepDevice();

            // Set up debounce after waking up
            is_debouncing = true;
            debounce_end_time = xTaskGetTickCount() + pdMS_TO_TICKS(screen_timeout_ms);
        }
    }
}

void GraphicsDriver::setupTouchDriver(TouchDriver &touchDriver) {
    // Initialize LVGL input device driver
    lv_indev_t * indev = lv_indev_create();        /* Create input device connected to Default Display. */
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);   /* Touch pad is a pointer-like device. */
    lv_indev_set_read_cb(indev, [](lv_indev_t *drv, lv_indev_data_t *data) {
        TouchDriver *touch = static_cast<TouchDriver*>(lv_indev_get_user_data(drv));
        touch->lvglRead(drv, data);
    }); 
}

void GraphicsDriver::init_backlight_pwm() {
    // Configure the PWM timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .timer_num        = PWM_TIMER,
        .freq_hz          = PWM_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // Configure the PWM channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = LCD_BL_GPIO,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = PWM_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = PWM_TIMER,
        .duty           = 0, // Start with backlight off
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

// Set the backlight brightness from 1 to 10
void GraphicsDriver::set_backlight_brightness(uint8_t brightness) {
    if (brightness < 1) brightness = 1;
    if (brightness > 10) brightness = 10;

    // Calculate duty cycle based on brightness level (1-10)
    uint32_t duty = (brightness * MAX_DUTY_CYCLE) / 10;
    
    // Set the duty cycle to adjust brightness
    ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL);

    ESP_LOGI(TAG, "Backlight brightness set to %d/10", brightness);
}

// New method to turn off the screen completely
void GraphicsDriver::turn_off_screen() {
    // Set the duty cycle to 0 (completely off)
    ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL);

    ESP_LOGI(TAG, "Screen turned off completely");
}