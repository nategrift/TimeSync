#include "VibrationDriver.h"

gpio_num_t VibrationDriver::vibrationPin = GPIO_NUM_33;
volatile bool VibrationDriver::stopVibration = false;
TaskHandle_t VibrationDriver::vibrationTaskHandle = NULL;
TaskHandle_t VibrationDriver::patternVibrationTaskHandle = NULL;

void VibrationDriver::init(gpio_num_t pin) {
    vibrationPin = pin;
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << vibrationPin);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_set_level(vibrationPin, 0);
}

void VibrationDriver::quickVibration(uint32_t duration_ms) {
    stopVibration = false;
    xTaskCreate(vibrationTask, "Quick Vibration", 2048, (void*)duration_ms, 5, &vibrationTaskHandle);
}

void VibrationDriver::patternVibration(const std::vector<uint32_t>& pattern) {
    stopVibration = false;
    std::vector<uint32_t>* patternCopy = new std::vector<uint32_t>(pattern);
    xTaskCreate(patternVibrationTask, "Pattern Vibration", 2048, (void*)patternCopy, 5, &patternVibrationTaskHandle);
}

void VibrationDriver::vibrationTask(void* pvParameters) {
    uint32_t duration = (uint32_t)pvParameters;
    gpio_set_level(vibrationPin, 1);
    for (uint32_t elapsed = 0; elapsed < duration && !stopVibration; elapsed += 10) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    gpio_set_level(vibrationPin, 0);
    vTaskDelete(NULL);
}

void VibrationDriver::patternVibrationTask(void* pvParameters) {
    std::vector<uint32_t>* pattern = static_cast<std::vector<uint32_t>*>(pvParameters);
    bool isOn = false;

    for (uint32_t duration : *pattern) {
        if (stopVibration) break;
        gpio_set_level(vibrationPin, isOn ? 1 : 0);
        for (uint32_t elapsed = 0; elapsed < duration && !stopVibration; elapsed += 10) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        isOn = !isOn;
    }

    gpio_set_level(vibrationPin, 0);
    delete pattern;
    vTaskDelete(NULL);
}

void VibrationDriver::incrementalVibration(uint32_t duration_ms) {
    stopVibration = false;
    std::vector<uint32_t> pattern;
    uint32_t interval = 700; // 0.7 seconds in milliseconds
    uint32_t elapsed = 0;

    while (elapsed < duration_ms) {
        pattern.push_back(interval); // On
        pattern.push_back(interval); // Off
        elapsed += 2 * interval;
    }

    patternVibration(pattern);
}

void VibrationDriver::stop() {
    stopVibration = true;

    if (vibrationTaskHandle != NULL) {
        vibrationTaskHandle = NULL;
    }

    if (patternVibrationTaskHandle != NULL) {
        patternVibrationTaskHandle = NULL;
    }

    gpio_set_level(vibrationPin, 0);
}