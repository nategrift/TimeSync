#pragma once

#include "IApp.h"
#include "AppManager.h"

class Fitness : public IApp {
private:
    AppManager& appManager;

    lv_obj_t* screenObj;
    lv_obj_t* stepsLabel;
    lv_obj_t* hourlyStepsLabel;
    lv_obj_t* stepsGoalArc;
    lv_obj_t* stepsHourlyGoalArc;

    TaskHandle_t stepsTaskHandle;
    static void stepsTaskWrapper(void* parameter);

public:
    Fitness(AppManager& manager);
    ~Fitness() override;

    void launch() override;
    void close() override;
    void backgroundActivity() override;
};
