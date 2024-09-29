#ifndef TIMER_H
#define TIMER_H

#include "IApp.h"
#include "AppManager.h"
#include "TimeManager.h"
#include "lvgl.h"
#include "ui_components.h"
#include "LVGLMutex.h"
#include "TimeEventsManager.h"
#include "settings_edit.h"

class Timer : public IApp {
private:
    lv_obj_t* screenObj;
    lv_obj_t* timerLabel;
    lv_obj_t* setTimerBtn;
    lv_obj_t* stateButton;
    AppManager& appManager;
    int timeListenerId;
    int remainingSeconds;
    time_t lastUpdateTime;
    lv_obj_t* previousScreen;

public:
    Timer(AppManager& manager);
    ~Timer() override;

    void launch() override;
    void close() override;
    void backgroundActivity() override;

private:
    void handleTimeUpdate(const struct tm& timeinfo);
    void updateTimerDisplay();
    void setTimer(int seconds);
    static void stateButtonHandler(lv_event_t* e);
    static void timerLabelClickHandler(lv_event_t* e);
    bool getTimer(TimeEvent& timeEvent);
    bool isTimerRunning();

    void startTimer();
    void stopTimer();
    void setTimerDuration(int seconds);
    void showSetTimerDialog();
};

#endif // TIMER_H
