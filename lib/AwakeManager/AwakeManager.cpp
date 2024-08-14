#include "AwakeManager.h"
#include "esp_sleep.h"
#include "esp_log.h"


extern "C" {
#include "gc9a01.h"
}

static const char *TAG = "AwakeManager";

#define TOUCH_INT_PIN GPIO_NUM_5

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
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(NULL);

    // Wait till all the SPI communications have been sent, then sleep
    vTaskDelay(pdMS_TO_TICKS(50));

    esp_sleep_enable_ext0_wakeup(TOUCH_INT_PIN, 0);
    esp_light_sleep_start();

    ESP_LOGI(TAG, "Waking up from sleep...");

    lv_disp_trig_activity(NULL);
    // Manually reset the inactivity timer by setting the last activity time to the current time
    lv_disp_t *disp = lv_disp_get_default();
    if (disp != NULL) {
        disp->last_activity_time = lv_tick_get();
    }
    wakeDevice();
}

void AwakeManager::wakeDevice() {
    ESP_LOGI(TAG, "Device woken up.");
    gc9a01_reload();
}
