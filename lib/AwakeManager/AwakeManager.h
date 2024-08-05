#ifndef AWAKE_MANAGER_H
#define AWAKE_MANAGER_H

#include "driver/gpio.h"
#include "lvgl.h"
#include "AppManager.h"

class AwakeManager {
public:
    static void init();
    static void sleepDevice(AppManager& appManager);
    static void wakeDevice(AppManager& appManager);
};

#endif // AWAKE_MANAGER_H