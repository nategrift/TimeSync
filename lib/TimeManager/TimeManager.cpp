#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "TimeManager.h"
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include <string>

static const char* TAG = "TimeManager";

static const int SERIALIZE_FREQUENCY = 1000;
static const int TIME_FREQUENCY = 50;

TimeManager::TimeManager(FileManager& fileManager) : fileManager(fileManager) {
    if (!deserializeTime()) {
        initializeTime();
    }
    xTaskCreate(timeTask, "TimeUpdateTask", 4096, this, 5, &timeTaskHandle);
}

void TimeManager::initializeTime() {
    time_t now;
    time(&now);
    setTime(now);
}

void TimeManager::updateTime() {
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);

    for (auto& [id, listener] : listeners) {
        listener(timeinfo);
    }
}

void TimeManager::setTime(time_t t) {
    timeval now = {.tv_sec = t, .tv_usec = 0};
    settimeofday(&now, NULL);

    updateTime();
}

void TimeManager::setDate(int year, int month, int day) {
    timeinfo.tm_year = year;
    timeinfo.tm_mon = month;
    timeinfo.tm_mday = day;

    time_t newTime = mktime(&timeinfo);
    setTime(newTime);
}

void TimeManager::timeTask(void *param) {
    auto *instance = static_cast<TimeManager *>(param);
    
    int serializeCount = SERIALIZE_FREQUENCY / TIME_FREQUENCY;
    while (1) {
        instance->updateTime();

        // if we should serialize, do so
        if (serializeCount <= 0) {
            instance->serializeTime(instance->timeinfo);
            serializeCount = SERIALIZE_FREQUENCY / TIME_FREQUENCY;
        }
        serializeCount--;

        vTaskDelay(pdMS_TO_TICKS(TIME_FREQUENCY));
    }
}

TimeManager::ListenerId TimeManager::addTimeUpdateListener(const TimeUpdateListener& listener) {
    ListenerId id = nextListenerId++;
    listeners[id] = listener;
    return id;
}

void TimeManager::removeTimeUpdateListener(ListenerId id) {
    listeners.erase(id);
}

void TimeManager::serializeTime(const struct tm& timeinfo) {
    std::string serializedData = std::to_string(timeinfo.tm_year) + " " +
                                 std::to_string(timeinfo.tm_mon) + " " +
                                 std::to_string(timeinfo.tm_mday) + " " +
                                 std::to_string(timeinfo.tm_hour) + " " +
                                 std::to_string(timeinfo.tm_min) + " " +
                                 std::to_string(timeinfo.tm_sec);
    fileManager.writeData("TimeManager", "time.txt", serializedData);
}

bool TimeManager::deserializeTime() {
    std::string data = fileManager.readData("TimeManager", "time.txt");
    if (data.empty()) {
        ESP_LOGI(TAG, "No saved time data found, using current time.");
        return false;
    }

    sscanf(data.c_str(), "%d %d %d %d %d %d",
           &timeinfo.tm_year,
           &timeinfo.tm_mon,
           &timeinfo.tm_mday,
           &timeinfo.tm_hour,
           &timeinfo.tm_min,
           &timeinfo.tm_sec);

    time_t restoredTime = mktime(&timeinfo);
    setTime(restoredTime);

    return true;
}
