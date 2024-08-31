#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "TimeManager.h"
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include <string>
#include "ConfigManager.h"

static const char* TAG = "TimeManager";

static const int SERIALIZE_FREQUENCY = 3000;
static const int TIME_FREQUENCY = 50;

struct tm TimeManager::timeinfo = {};
std::map<ListenerId, TimeUpdateListener> TimeManager::listeners = {};
ListenerId TimeManager::nextListenerId = 0;
TaskHandle_t timeTaskHandle = nullptr;


void TimeManager::init() {
    TimeManager::nextListenerId = 0;
    if (!TimeManager::deserializeTime()) {
        TimeManager::initializeTime();
    }
}

void TimeManager::initializeTime() {
    time_t now;
    time(&now);
    TimeManager::setRTCTime(now);
}

const tm TimeManager::getTimeInfo() {
    return TimeManager::timeinfo;
}

void TimeManager::updateTime() {
    time_t now;
    time(&now);
    localtime_r(&now, &TimeManager::timeinfo);

    for (auto& [id, listener] : TimeManager::listeners) {
        listener(TimeManager::timeinfo);
    }
}

void TimeManager::setRTCTime(time_t t) {
    timeval now = {.tv_sec = t, .tv_usec = 0};
    settimeofday(&now, NULL);

    TimeManager::updateTime();
}

void TimeManager::setDate(int year, int month, int day) {
    TimeManager::timeinfo.tm_year = year - 1900; // based minus 1900
    TimeManager::timeinfo.tm_mon = month - 1; // month is base 0
    TimeManager::timeinfo.tm_mday = day;

    time_t newTime = mktime(&TimeManager::timeinfo);
    TimeManager::setRTCTime(newTime);
    TimeManager::serializeTime(TimeManager::timeinfo);
}

void TimeManager::setTime(int hour, int minute, int second) {
    TimeManager::timeinfo.tm_hour = hour;
    TimeManager::timeinfo.tm_min = minute;
    TimeManager::timeinfo.tm_sec = second;

    time_t newTime = mktime(&TimeManager::timeinfo);
    TimeManager::setRTCTime(newTime);
    TimeManager::serializeTime(TimeManager::timeinfo);
}

void TimeManager::getTime(int &hour, int &minute, int &second) {
    hour = TimeManager::timeinfo.tm_hour;
    minute = TimeManager::timeinfo.tm_min;
    second = TimeManager::timeinfo.tm_sec;
}

void TimeManager::timeTask(void* params) {
    int serializeCount = SERIALIZE_FREQUENCY / TIME_FREQUENCY;
    while (1) {
        TimeManager::updateTime();

        // if we should serialize, do so
        if (serializeCount <= 0) {
            TimeManager::serializeTime(TimeManager::timeinfo);
            serializeCount = SERIALIZE_FREQUENCY / TIME_FREQUENCY;
        }
        serializeCount--;

        vTaskDelay(pdMS_TO_TICKS(TIME_FREQUENCY));
    }
}

ListenerId TimeManager::addTimeUpdateListener(const TimeUpdateListener& listener) {
    ListenerId id = TimeManager::nextListenerId++;
    TimeManager::listeners[id] = listener;
    return id;
}

void TimeManager::removeTimeUpdateListener(ListenerId id) {
    TimeManager::listeners.erase(id);
}

void TimeManager::serializeTime(const struct tm& timeinfo) {

    std::string timeSerializedData = std::to_string(timeinfo.tm_hour) + ":" +
                                 std::to_string(timeinfo.tm_min) + ":" +
                                 std::to_string(timeinfo.tm_sec);

    std::string dateSerializedData = std::to_string(timeinfo.tm_year + 1900) + "-" +
                                 std::to_string(timeinfo.tm_mon + 1) + "-" +
                                 std::to_string(timeinfo.tm_mday);

    ConfigManager::setConfigString("General", "Time", timeSerializedData);
    ConfigManager::setConfigString("General", "Date", dateSerializedData);
}

bool TimeManager::deserializeTime() {
    std::string time = ConfigManager::getConfigString("General", "Time");
    std::string date = ConfigManager::getConfigString("General", "Date");
    if (time.empty() || date.empty()) {
        ESP_LOGI(TAG, "No saved time data found, using current time.");
        return false;
    }

    sscanf(time.c_str(), "%d:%d:%d",
           &timeinfo.tm_hour,
           &timeinfo.tm_min,
           &timeinfo.tm_sec);

     sscanf(date.c_str(), "%d-%d-%d",
           &timeinfo.tm_year,
           &timeinfo.tm_mon,
           &timeinfo.tm_mday);
    timeinfo.tm_year = timeinfo.tm_year - 1900;
    timeinfo.tm_mon = timeinfo.tm_mon - 1;

    time_t restoredTime = mktime(&timeinfo);
    TimeManager::setRTCTime(restoredTime);

    return true;
}
