#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <map>
#include <functional>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <ctime>
#include <string>
#include "FileManager.h"

using TimeUpdateListener = std::function<void(const struct tm&)>;
using ListenerId = int;

class TimeManager {

private:
    static std::map<ListenerId, TimeUpdateListener> listeners;
    static ListenerId nextListenerId;
    static TaskHandle_t timeTaskHandle;

public:
    static void init();
    static const tm getLocalTimeInfo();
    static const tm getUTCTimeInfo();
    static void updateTime();
    static void setRTCTime(time_t t);
    static void setTime(int hour, int minute, int second);
    static void getTime(int &hour, int &minute, int &second);
    static void setDate(int year, int month, int day);
    static void timeTask(void* params);
    static ListenerId addTimeUpdateListener(const TimeUpdateListener& listener);
    static void removeTimeUpdateListener(ListenerId id);

    static void serializeTime();
    static bool deserializeTime();
};

#endif // TIMEMANAGER_H
