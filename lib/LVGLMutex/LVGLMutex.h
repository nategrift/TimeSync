#ifndef LVGL_MUTEX_H
#define LVGL_MUTEX_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class LvglMutex {
public:
    // Initialize the mutex
    static void init() {
        if (mutex == nullptr) {
            mutex = xSemaphoreCreateMutex();
        }
    }

    // Lock the mutex
    static void lock() {
        if (mutex != nullptr) {
            xSemaphoreTake(mutex, portMAX_DELAY);
        }
    }

    // Unlock the mutex
    static void unlock() {
        if (mutex != nullptr) {
            xSemaphoreGive(mutex);
        }
    }

private:
    // Private constructor to prevent instantiation
    LvglMutex() {}

    static SemaphoreHandle_t mutex; // Static mutex handle
};

#endif // LVGL_MUTEX_H
