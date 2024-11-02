#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "TimeManager.h"
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include <string>
#include "ConfigManager.h"
#include "LVGLMutex.h"

static const char* TAG = "TimeManager";

static const int SERIALIZE_FREQUENCY = 3000;
static const int TIME_FREQUENCY = 50;

std::map<ListenerId, TimeUpdateListener> TimeManager::listeners = {};
ListenerId TimeManager::nextListenerId = 0;
TaskHandle_t timeTaskHandle = nullptr;


void TimeManager::init() {
    TimeManager::nextListenerId = 0;
    TimeManager::deserializeTime();

    // Set timezone to Mountain Time
    setenv("TZ", "MST7MDT,M3.2.0,M11.1.0", 1);
    tzset();
    ESP_LOGI(TAG, "Timezone set to Mountain Time");
}

const tm TimeManager::getLocalTimeInfo() {
    time_t now;
    time(&now);
    tm timeinfo;
    localtime_r(&now, &timeinfo);
    return timeinfo;
}

const tm TimeManager::getUTCTimeInfo() {
    time_t now;
    time(&now);
    tm timeinfo;
    gmtime_r(&now, &timeinfo);
    return timeinfo;
}

void TimeManager::updateTime() {
    tm timeinfo = TimeManager::getLocalTimeInfo();

    for (auto& [id, listener] : TimeManager::listeners) {
        listener(timeinfo);
    }
}

void TimeManager::setRTCTime(time_t t) {
    timeval now = {.tv_sec = t, .tv_usec = 0};
    settimeofday(&now, NULL);

    TimeManager::updateTime();
}

void TimeManager::setDate(int year, int month, int day) {
    tm timeinfo = TimeManager::getLocalTimeInfo();
    timeinfo.tm_year = year - 1900;  // Convert to years since 1900
    timeinfo.tm_mon = month - 1;     // Convert to 0-based month
    timeinfo.tm_mday = day;

    // mktime() automatically converts local time to UTC timestamp
    time_t utcTime = mktime(&timeinfo);
    TimeManager::setRTCTime(utcTime);  // RTC is always set in UTC
    TimeManager::serializeTime();
}

void TimeManager::setTime(int hour, int minute, int second) {
    tm timeinfo = TimeManager::getLocalTimeInfo();
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;

    time_t newTime = mktime(&timeinfo);
    TimeManager::setRTCTime(newTime);
    TimeManager::serializeTime();
}

void TimeManager::getTime(int &hour, int &minute, int &second) {
    tm timeinfo = TimeManager::getLocalTimeInfo();
    hour = timeinfo.tm_hour;
    minute = timeinfo.tm_min;
    second = timeinfo.tm_sec;
}

void TimeManager::timeTask(void* params) {
    int serializeCount = SERIALIZE_FREQUENCY / TIME_FREQUENCY;
    while (1) {
        LvglMutex::lock();
        TimeManager::updateTime();
        LvglMutex::unlock();
        
        // if we should serialize, do so
        if (serializeCount <= 0) {
            TimeManager::serializeTime();
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

void TimeManager::serializeTime() {
    tm timeinfo = TimeManager::getUTCTimeInfo();
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
    tm timeinfo = TimeManager::getUTCTimeInfo();
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
