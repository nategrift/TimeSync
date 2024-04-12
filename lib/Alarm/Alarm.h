#ifndef ALARM_H
#define ALARM_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "TextComponent.h"
#include "ListComponent.h"
#include "UIManager.h"
#include <memory>
#include <vector>
#include "AppManager.h"
#include "IApp.h"

class Alarm : public IApp  {
public:
    Alarm(AppManager& manager);
    ~Alarm();

    void launch();
    void close();
    void clearUI();
    void setAlarm(int hour, int minute);
    void setAlarmEnabled(int hour, int minute, bool enabled);
    void deleteAlarm(int hour, int minute);
    bool isAlarmOn() const;

    void serializeAlarms();
    void deserializeAlarms();

    struct AlarmTime {
        int hour;
        int minute;
        bool enabled;
    };


private:
    TaskHandle_t alarmCheckTaskHandle;
    std::vector<int> activeComponents;
    std::vector<AlarmTime> alarms;
    int selectedAlarmId;

    enum class AlarmState {
        DEFAULT,
        NEW,
        OPTIONS
    };

    AlarmState currentState;

    void showDefaultState();
    void enterNewAlarmState();
    void enterOptionsState();
    void transitionToState(AlarmState newState);
    void backgroundActivity();
    static void alarmCheckTask(void* params);
};

#endif // ALARM_H
