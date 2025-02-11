#ifndef CLOCK_H
#define CLOCK_H

#include "lvgl.h"
#include "IApp.h"
#include "AppManager.h"
#include "TimeManager.h"
#include "BatteryManager.h"
#include "ui_components.h"
#include "LVGLMutex.h"

class Clock : public IApp {
private:
    int timeListenerId;
    lv_obj_t* clockTimeLabel;
    lv_obj_t* clockDateLabel;
    lv_obj_t* screenObj;
    lv_obj_t* batteryLabel;
    lv_obj_t* batteryIcon;
    lv_obj_t* wifiIcon;
    lv_obj_t* stepsLabel;
    AppManager& appManager;
    BatteryManager& batteryManager;
    lv_timer_t* batteryUpdateTimer;
    lv_timer_t* wifiUpdateTimer;
    bool isClosing;

public:
    Clock(AppManager& manager);
    ~Clock() override;

    void launch() override;
    void close() override;
    void backgroundActivity() override;

private:
    void handleTimeUpdate(const struct tm& timeinfo);
    void updateBatteryLevel();
    void updateWifiIcon();
    void updateSteps();
};
#endif // CLOCK_H

