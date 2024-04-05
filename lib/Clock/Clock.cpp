#include "esp_log.h"
#include "TextComponent.h"
#include <memory>

#include "Clock.h"
#include "UIManager.h"

Clock::Clock(AppManager& manager) : IApp(manager), timeListenerId(-1) {}


Clock::~Clock() {
    if (timeListenerId != -1) {
            TimeManager& timeManager = appManager.getTimeManager();
            timeManager.removeTimeUpdateListener(timeListenerId);
        }
}

void Clock::launch() {
    UIManager& uiManager = appManager.getUIManager();
    auto timeComponent = std::make_shared<TextComponent>("--:--");
    clockTime_UI = uiManager.addOrUpdateComponent(timeComponent);
    auto titleComponent = std::make_shared<TextComponent>("Clock");
    clockTitle_UI = uiManager.addOrUpdateComponent(titleComponent);

    TimeManager& timeManager = appManager.getTimeManager();
    timeListenerId = timeManager.addTimeUpdateListener([this](const struct tm& timeinfo) {
        this->handleTimeUpdate(timeinfo);
    });
}

void Clock::close() {
    if (timeListenerId != -1) {
        TimeManager& timeManager = appManager.getTimeManager();
        timeManager.removeTimeUpdateListener(timeListenerId);
        timeListenerId = -1;
    }
    
    UIManager& uiManager = appManager.getUIManager();
    uiManager.deleteComponent(clockTime_UI);
    uiManager.deleteComponent(clockTitle_UI);
}

void Clock::handleTimeUpdate(const struct tm& timeinfo) {
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%I:%M:%S %p", &timeinfo);

    UIManager& uiManager = appManager.getUIManager();
    uiManager.updateComponentText(clockTime_UI, std::string(strftime_buf));
}

void Clock::backgroundActivity() {
    // currently nothing, 
}
