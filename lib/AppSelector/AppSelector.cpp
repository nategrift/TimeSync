#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "TextComponent.h"
#include "UIManager.h"
#include <memory>

#include "AppSelector.h"

AppSelector::AppSelector(AppManager& manager) : IApp(manager), listComponentId(-1) {
    
}

AppSelector::~AppSelector() {
}

void AppSelector::launch() {

    UIManager& uiManager = appManager.getUIManager();
    std::shared_ptr<ListComponent> appList = std::make_shared<ListComponent>(appManager);
    appList->addItem(0, std::make_shared<TextComponent>("Clock"));
    appList->addItem(1, std::make_shared<TextComponent>("Alarm"));
    appList->addItem(2, std::make_shared<TextComponent>("Stopwatch"));

    appList->setOnSelectCallback([this](int appId) {
        ESP_LOGI("AppSelector", "AppSelector chooses app id of %d", appId);
        switch (appId) {
            case 0: appManager.launchApp("Clock"); break;
            case 1: appManager.launchApp("Alarm"); break;
            case 2: appManager.launchApp("StopWatch"); break;
            default: break;
        }
    });


    listComponentId = uiManager.addOrUpdateComponent(appList);
}

void AppSelector::close() {
    UIManager& uiManager = appManager.getUIManager();
    uiManager.deleteComponent(listComponentId);
}

void AppSelector::backgroundActivity() {
    // nothing
}