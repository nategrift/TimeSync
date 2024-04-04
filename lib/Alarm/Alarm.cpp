#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "TextComponent.h"
#include "UIManager.h"

#include "Alarm.h"

Alarm::Alarm(AppManager& manager) : IApp(manager), isLaunched(false), alarmCheckTaskHandle(nullptr) {}

Alarm::~Alarm() {
    // if for some reason we already have a task, remove it
    if (alarmCheckTaskHandle != nullptr) {
        vTaskDelete(alarmCheckTaskHandle);
    }
}

void Alarm::launch() {
    isLaunched = true;

    UIManager& uiManager = appManager.getUIManager();
    alarmTime_UI = uiManager.addOrUpdateComponent(TextComponent("--:--", true));
    alarmTitle_UI = uiManager.addOrUpdateComponent(TextComponent("Add An Alarm.", true));
}

void Alarm::close() {
    isLaunched = false;

    UIManager& uiManager = appManager.getUIManager();
    uiManager.deleteComponent(alarmTime_UI);
    uiManager.deleteComponent(alarmTitle_UI);
}

void Alarm::backgroundActivity() {
    while(1) {
        time_t now = time(NULL);
        struct tm currentTime;
        localtime_r(&now, &currentTime);

        for (auto& alarm : alarms) {
            if (currentTime.tm_hour == alarm.hour && currentTime.tm_min == alarm.minute) {
                // Time for alarm to go off
                // TODO: add buzzing and maybe a notification
                ESP_LOGI("Alarm", "An Alarm is going off....");
            }
        }

        // wait 30 seconds, we only set it based on the hour and minute. Eventually we should really be setting this to be a listener on minute change from timer
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

void Alarm::setAlarm(int hour, int minute) {
    alarms.push_back({hour, minute});

    xTaskCreate(alarmCheckTask, "AlarmCheckTask", 2048, this, 5, &alarmCheckTaskHandle);
}

void Alarm::deleteAlarm(int hour, int minute) {
    alarms.erase(std::remove_if(alarms.begin(), alarms.end(), [hour, minute](const AlarmTime& alarm) {
        return alarm.hour == hour && alarm.minute == minute;
    }), alarms.end());

    // if no more alarms, then delete the alarm
    if (alarms.empty() && alarmCheckTaskHandle != nullptr) {
        vTaskDelete(alarmCheckTaskHandle);
        alarmCheckTaskHandle = nullptr;
    }
}

bool Alarm::isAlarmOn() const {
    // Return true if there's an alarm set
    return !alarms.empty();
}

void Alarm::alarmCheckTask(void* params) {
    auto* alarmApp = static_cast<Alarm*>(params);
    alarmApp->backgroundActivity();
}
