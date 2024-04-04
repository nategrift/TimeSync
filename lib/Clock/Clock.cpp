#include "esp_log.h"
#include "TextComponent.h"

#include "Clock.h"
#include "UIManager.h"

Clock::Clock(AppManager& manager) : IApp(manager), timeListenerId(-1) {}


Clock::~Clock() {
    if (timeListenerId != -1) {
            TimeManager& timeManager = appManager.getTimeManager();  // Assuming AppManager can provide TimeManager.
            timeManager.removeTimeUpdateListener(timeListenerId);
        }
}

void Clock::launch() {
    UIManager& uiManager = appManager.getUIManager();
    clockTime_UI = uiManager.addOrUpdateComponent(TextComponent("--:--", true));
    clockTitle_UI = uiManager.addOrUpdateComponent(TextComponent("Clock", true));

    TimeManager& timeManager = appManager.getTimeManager();
    timeListenerId = timeManager.addTimeUpdateListener([this](const struct tm& timeinfo) {
        this->handleTimeUpdate(timeinfo);
    });
}

void Clock::close() {
    UIManager& uiManager = appManager.getUIManager();
    uiManager.deleteComponent(clockTime_UI);
    uiManager.deleteComponent(clockTitle_UI);

    if (timeListenerId != -1) {
        TimeManager& timeManager = appManager.getTimeManager();
        timeManager.removeTimeUpdateListener(timeListenerId);
        timeListenerId = -1;
    }
}

void Clock::handleTimeUpdate(const struct tm& timeinfo) {
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%I:%M:%S %p", &timeinfo);

    ESP_LOGI("TimeManager", "New Time: %s", strftime_buf);
    UIManager& uiManager = appManager.getUIManager();
    uiManager.updateComponentText(clockTime_UI, std::string(strftime_buf));
}

void Clock::backgroundActivity() {
    // currently nothing, 
}
