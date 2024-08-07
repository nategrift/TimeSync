#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <map>
#include <functional>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <ctime>
#include <string>
#include "FileManager.h"

class TimeManager {
public:
    using TimeUpdateListener = std::function<void(const struct tm&)>;
    using ListenerId = int;

private:
    std::map<ListenerId, TimeUpdateListener> listeners;
    ListenerId nextListenerId = 0;
    TaskHandle_t timeTaskHandle = nullptr;
    FileManager& fileManager;
    struct tm timeinfo;

    void serializeTime(const struct tm& timeinfo);
    bool deserializeTime();

public:
    TimeManager(FileManager& fileManager);
    void initializeTime();
    void updateTime();
    void setTime(time_t t);
    void setDate(int year, int month, int day);
    static void timeTask(void *param);
    ListenerId addTimeUpdateListener(const TimeUpdateListener& listener);
    void removeTimeUpdateListener(ListenerId id);
};

#endif // TIMEMANAGER_H
