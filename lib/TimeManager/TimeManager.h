#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include "UIManager.h"
#include "ComponentID.h"
#include "driver/gpio.h"
#include <ctime>

class TimeManager {
private:
    UIManager& uiManager;

    void initializeTime();

    void updateTime();

public:
    TimeManager(UIManager& uiManager);

    void setTime(time_t t);

    static void timeTask(void *param);
};

#endif // TIMEMANAGER_H
