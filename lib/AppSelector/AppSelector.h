#ifndef APP_SELECTOR_H
#define APP_SELECTOR_H

#include "AppManager.h"
#include "IApp.h"
#include "lvgl.h"

class AppSelector : public IApp {
private:
    AppManager& appManager;
    lv_obj_t* screenObj;

public:
    AppSelector(AppManager& manager);
    ~AppSelector() override;

    void launch() override;
    void close() override;
    void backgroundActivity() override;
};

#endif // APP_SELECTOR_H
