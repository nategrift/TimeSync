#ifndef ALARM_H
#define ALARM_H

#include <vector>
#include <functional>
#include <ctime>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "AppManager.h"
#include "IApp.h"

class Alarm : public IApp {
private:
    struct AlarmTime {
        int hour;
        int minute;
    };

    std::vector<AlarmTime> alarms;
    bool isLaunched;
    int alarmTime_UI;
    int alarmTitle_UI;
public:
    Alarm(AppManager& manager);
    ~Alarm() override;

    void launch() override;
    void close() override;
    void backgroundActivity() override;

    void setAlarm(int hour, int minute);
    void deleteAlarm(int hour, int minute);
    bool isAlarmOn() const;
    static void alarmCheckTask(void* params);
private:
    TaskHandle_t alarmCheckTaskHandle;

};

#endif // ALARM_H
