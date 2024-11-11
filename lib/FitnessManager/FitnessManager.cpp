#include "FitnessManager.h"
#include "ConfigManager.h"
#include "MotionDriver.h"
#include "FileManager.h"
#include "TimeManager.h"
#include "esp_log.h"
#include <sstream>

namespace FitnessManager {
    bool is_new_day(time_t currentTime, time_t lastTime) {
        struct tm current_tm;
        struct tm last_tm;
        gmtime_r(&currentTime, &current_tm);  // Thread-safe version
        gmtime_r(&lastTime, &last_tm);        // Thread-safe version
        
        return (current_tm.tm_mday != last_tm.tm_mday || 
                current_tm.tm_mon != last_tm.tm_mon || 
                current_tm.tm_year != last_tm.tm_year);
    }

    bool is_new_hour(time_t currentTime, time_t lastTime) {
        struct tm current_tm;
        struct tm last_tm;
        gmtime_r(&currentTime, &current_tm);  // Thread-safe version
        gmtime_r(&lastTime, &last_tm);        // Thread-safe version
        
        return (current_tm.tm_hour != last_tm.tm_hour ||
                current_tm.tm_mday != last_tm.tm_mday ||
                current_tm.tm_mon != last_tm.tm_mon ||
                current_tm.tm_year != last_tm.tm_year);
    }

    void handle_daily_steps(int currentSteps) {
        tm timeinfo = TimeManager::getLocalTimeInfo();
        // Convert timeinfo to time_t (seconds since epoch)
        time_t currentTime = mktime(&timeinfo);
        
        // Get daily steps
        int dailySteps = ConfigManager::getConfigInt(FitnessManager::KEY, FitnessManager::DAILY_STEPS_KEY);
        time_t lastFetchLocal = ConfigManager::getConfig64(FitnessManager::KEY, FitnessManager::LAST_FETCH_LOCAL_KEY);

        ESP_LOGI(FitnessManager::TAG, "Comparing times - Current: %lld, Last: %lld", currentTime, lastFetchLocal);

        if (is_new_day(currentTime, lastFetchLocal)) {
            ESP_LOGI(FitnessManager::TAG, "New day detected, resetting daily steps");
            dailySteps = 0;
        }

        dailySteps += currentSteps;

        ESP_LOGI(FitnessManager::TAG, "Current daily steps: %d", dailySteps);
        ConfigManager::setConfigInt(FitnessManager::KEY, FitnessManager::DAILY_STEPS_KEY, dailySteps);
        ConfigManager::setConfig64(FitnessManager::KEY, FitnessManager::LAST_FETCH_LOCAL_KEY, currentTime);
    }

    void handle_hourly_steps(int currentSteps) {
        tm timeinfo = TimeManager::getUTCTimeInfo();
        time_t currentUTCTime = mktime(&timeinfo);

        time_t lastFetchUTCTime = ConfigManager::getConfig64(FitnessManager::KEY, FitnessManager::LAST_FETCH_UTC_KEY);
        int currentHourSteps = ConfigManager::getConfigInt(FitnessManager::KEY, FitnessManager::CURRENT_HOUR_STEPS_KEY);

        ESP_LOGI(FitnessManager::TAG, "Comparing UTC times - Current: %lld, Last: %lld", currentUTCTime, lastFetchUTCTime);

        if (is_new_hour(currentUTCTime, lastFetchUTCTime)) {
            ESP_LOGI(FitnessManager::TAG, "New UTC hour detected, saving hour data");

            std::stringstream ss;
            // Format: UTC_timestamp,steps\n
            ss << lastFetchUTCTime << "," << currentHourSteps << "\n";
            
            // Append to the hourly log file
            FileManager::appendData("fitness", "hourly_steps.txt", ss.str());
            ESP_LOGI(FitnessManager::TAG, "Saved UTC hour steps to file: timestamp=%lld, steps=%d", 
                    currentUTCTime, currentHourSteps);

            currentHourSteps = 0;
        }

        currentHourSteps += currentSteps;
        
        ESP_LOGI(FitnessManager::TAG, "Current hourly steps: %d", currentHourSteps);
        ConfigManager::setConfigInt(FitnessManager::KEY, FitnessManager::CURRENT_HOUR_STEPS_KEY, currentHourSteps);
        ConfigManager::setConfig64(FitnessManager::KEY, FitnessManager::LAST_FETCH_UTC_KEY, currentUTCTime);
    }

    void handle_fitness_task(void* params) {
        while (1) {
            u_int32_t steps = 0;
            MotionDriver::readStepCount(steps);
            handle_daily_steps(steps);
            handle_hourly_steps(steps);
            MotionDriver::resetStepCount();

            vTaskDelay(pdMS_TO_TICKS(5000)); // 30 second delay
        }
    }
}