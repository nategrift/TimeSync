#ifndef CLOCK_H
#define CLOCK_H

#include <vector>
#include <functional>
#include <ctime>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "AppManager.h"
#include "IApp.h"
#include "lvgl.h"

class Clock : public IApp {
private:
    TimeManager::ListenerId timeListenerId;
    lv_obj_t* clockTimeLabel;
    lv_obj_t* clockTitleLabel;
    lv_obj_t* batteryLabel;
    lv_obj_t* batteryIcon;
    BatteryManager& batteryManager;

    void handleTimeUpdate(const struct tm& timeinfo);
    void updateBatteryLevel();

public:
    Clock(AppManager& manager);
    ~Clock() override;

    void launch() override;
    void close() override;
    void backgroundActivity() override;
};

#endif // CLOCK_H
