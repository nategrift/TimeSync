#ifndef AWAKE_MANAGER_H
#define AWAKE_MANAGER_H

#include "driver/gpio.h"
#include "lvgl.h"

class AwakeManager {
public:
    static void init();
    static void sleepDevice();
    static void wakeDevice(int before_sleep_time);
};

#endif // AWAKE_MANAGER_H