#ifndef CLOCK_H
#define CLOCK_H

#include <vector>
#include <functional>
#include <ctime>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "AppManager.h"
#include "IApp.h"
#include <memory>

class Clock : public IApp {
private:
    int clockTitle_UI;
    int clockTime_UI;
    TimeManager::ListenerId timeListenerId;

    void handleTimeUpdate(const struct tm& timeinfo);

public:
    Clock(AppManager& manager);
    ~Clock() override;

    void launch() override;
    void close() override;
    void backgroundActivity() override;
};

#endif // CLOCK_H
