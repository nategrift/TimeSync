#include "AwakeManager.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include <vector>

extern "C" {
#include "gc9a01.h"
#include "display.h"
}

static const char *TAG = "AwakeManager";

#define TOUCH_INT_PIN GPIO_NUM_5

static std::vector<lv_timer_t*> paused_timers;

// Function to pause all timers
void pause_all_lvgl_timers() {
    lv_timer_t* timer = lv_timer_get_next(NULL);
    while (timer != NULL) {
        // Store the active timer
        paused_timers.push_back(timer);

        // Stop the timer
        lv_timer_pause(timer);

        // Get the next timer
        timer = lv_timer_get_next(timer);
    }
}

// Function to resume all paused timers
void resume_all_lvgl_timers() {
    for (lv_timer_t* timer : paused_timers) {
        // Resume the timer
        lv_timer_resume(timer);
    }

    // Clear the list after resuming
    paused_timers.clear();
}

void AwakeManager::init() {
    // touch interrupt for waking up
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1ULL << TOUCH_INT_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
}

void AwakeManager::sleepDevice() {
    ESP_LOGI(TAG, "Entering light sleep...");

    pause_lvgl_tick_timer();

    pause_all_lvgl_timers();
    
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(NULL);

    // Wait till all the SPI communications have been sent, then sleep
    vTaskDelay(pdMS_TO_TICKS(50));

    int64_t before_sleep_time = esp_timer_get_time();

    esp_sleep_enable_ext0_wakeup(TOUCH_INT_PIN, 0);
    esp_light_sleep_start();

    wakeDevice(before_sleep_time);
}

void AwakeManager::wakeDevice(int before_sleep_time) {
    ESP_LOGI(TAG, "Device woken up.");
    gc9a01_reload();
    resume_lvgl_tick_timer();

    resume_all_lvgl_timers();

    ESP_LOGI(TAG, "Waking up from sleep...");

    uint64_t after_sleep_time = esp_timer_get_time(); // Time after waking up
    uint64_t sleep_duration = (after_sleep_time - before_sleep_time) / 1000; // Convert to milliseconds

    lv_tick_inc(sleep_duration); // Adjust LVGL tick count

    lv_disp_trig_activity(NULL);
    // Manually reset the inactivity timer by setting the last activity time to the current time
    lv_disp_t *disp = lv_disp_get_default();
    if (disp != NULL) {
        disp->last_activity_time = lv_tick_get();
    }
}
