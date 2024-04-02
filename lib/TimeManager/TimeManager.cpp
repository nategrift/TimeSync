#include "TimeManager.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "esp32/rom/rtc.h"
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

TimeManager::TimeManager(UIManager& uiManager) 
    : uiManager(uiManager) {
    initializeTime();
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
    // Update the time from the RTC esp module
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    char strftime_buf[64];
    // %H:%M:%S for 24 hr format
    // %H:%M:%S %p is 12 hr with am/pm
    strftime(strftime_buf, sizeof(strftime_buf), "%H:%M:%S %p", &timeinfo);

    ESP_LOGI("TimeManager", "New Time: %s", strftime_buf);

    uiManager.updateComponentText(ComponentID::Time, std::string(strftime_buf));
}

void TimeManager::setTime(time_t t) {
    timeval now = {.tv_sec = t, .tv_usec = 0};
    settimeofday(&now, NULL);

    updateTime();
}

void TimeManager::timeTask(void *param) {
    auto *instance = static_cast<TimeManager *>(param);
    while(1)
    {
        instance->updateTime();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


