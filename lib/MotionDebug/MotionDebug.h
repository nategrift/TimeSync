#ifndef MOTIONDEBUG_H
#define MOTIONDEBUG_H

#include "IApp.h"
#include "AppManager.h"
#include <string>
#include "LVGLMutex.h"
#include "MotionDriver.h"

class MotionDebug : public IApp {
private:
    AppManager& appManager;
    MotionDriver motionDriver;
    lv_obj_t* screenObj;
    static void updateTask(void* parameter);
    TaskHandle_t updateTaskHandle;
    bool isRunning;

public:
    MotionDebug(AppManager& manager);
    ~MotionDebug() override;

    void launch() override;
    void close() override;
    void backgroundActivity() override;
    
    static void updateDisplay(lv_timer_t *timer);
    static void refresh_event_handler(lv_event_t *event);
    static lv_obj_t* textArea;
    static lv_obj_t *circle;
    static lv_obj_t *refreshBtn;
};

#endif // MOTIONDEBUG_H
