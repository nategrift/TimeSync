#include "VibrationDriver.h"

gpio_num_t VibrationDriver::vibrationPin = GPIO_NUM_33;

void VibrationDriver::init(gpio_num_t pin) {
    vibrationPin = pin;
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << vibrationPin);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
}

void VibrationDriver::quickVibration(uint32_t duration_ms) {
    xTaskCreate(vibrationTask, "Quick Vibration", 2048, (void*)duration_ms, 5, NULL);
}

void VibrationDriver::patternVibration(const std::vector<uint32_t>& pattern) {
    std::vector<uint32_t>* patternCopy = new std::vector<uint32_t>(pattern);
    xTaskCreate(patternVibrationTask, "Pattern Vibration", 2048, (void*)patternCopy, 5, NULL);
}

void VibrationDriver::vibrationTask(void* pvParameters) {
    uint32_t duration = (uint32_t)pvParameters;
    gpio_set_level(vibrationPin, 1);
    vTaskDelay(pdMS_TO_TICKS(duration));
    gpio_set_level(vibrationPin, 0);
    vTaskDelete(NULL);
}

void VibrationDriver::patternVibrationTask(void* pvParameters) {
    std::vector<uint32_t>* pattern = static_cast<std::vector<uint32_t>*>(pvParameters);
    bool isOn = false;

    for (uint32_t duration : *pattern) {
        gpio_set_level(vibrationPin, isOn ? 1 : 0);
        vTaskDelay(pdMS_TO_TICKS(duration));
        isOn = !isOn;
    }

    gpio_set_level(vibrationPin, 0);
    delete pattern;
    vTaskDelete(NULL);
}