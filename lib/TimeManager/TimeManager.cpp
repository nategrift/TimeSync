#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "TimeManager.h"
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"

TimeManager::TimeManager() {
    initializeTime();
    xTaskCreate(timeTask, "TimeUpdateTask", 4096, this, 5, &timeTaskHandle);
}

void TimeManager::initializeTime() {
    // Placeholder time ( retreive from smartphone later )
    struct tm timeinfo = {};
    timeinfo.tm_year = 121;
    timeinfo.tm_mon = 0;
    timeinfo.tm_mday = 1;
    time_t t = mktime(&timeinfo);
    timeval now = {.tv_sec = t, .tv_usec = 0};
    settimeofday(&now, NULL);
}

void TimeManager::updateTime() {
    time_t now;
    struct tm timeinfo;
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

void TimeManager::timeTask(void *param) {
    auto *instance = static_cast<TimeManager *>(param);
    while (1) {
        instance->updateTime();
        vTaskDelay(pdMS_TO_TICKS(50));
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
