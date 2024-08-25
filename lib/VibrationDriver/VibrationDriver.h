
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gpio.h"
#include <vector>

class VibrationDriver {
public:
    static void init(gpio_num_t pin = GPIO_NUM_33);
    static void quickVibration(uint32_t duration_ms = 100);
    static void patternVibration(const std::vector<uint32_t>& pattern);

private:
    static gpio_num_t vibrationPin;
    static void vibrationTask(void* pvParameters);
    static void patternVibrationTask(void* pvParameters);
};