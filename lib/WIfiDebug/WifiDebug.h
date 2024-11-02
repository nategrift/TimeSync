#ifndef WIFIDEBUG_H
#define WIFIDEBUG_H

#include "IApp.h"
#include "AppManager.h"
#include <string>
#include "LVGLMutex.h"

class WifiDebug : public IApp {
private:
    AppManager& appManager;
    lv_obj_t* screenObj;
    lv_obj_t* textArea;
    lv_obj_t* refreshBtn;

public:
    WifiDebug(AppManager& manager);
    ~WifiDebug() override;

    void launch() override;
    void close() override;
    void backgroundActivity() override;
    
    static void refresh_event_handler(lv_event_t* e);
    void updateDisplay();
};

#endif // WIFIDEBUG_H
