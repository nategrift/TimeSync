#pragma once

#include <time.h>

/**
 * @brief Handles fitness tracking functionality
 */
namespace FitnessManager {
    static const char* TAG = "FitnessManager";

    // Config keys
    static const char* CURRENT_HOUR_STEPS_KEY = "current_hour_steps";
    static const char* LAST_FETCH_UTC_KEY = "last_fetch_utc";

    static const char* DAILY_STEPS_KEY = "daily_steps";
    static const char* LAST_FETCH_LOCAL_KEY = "last_fetch_local";

    static const char* KEY = "Fitness";
    /**
     * @brief Main task function for handling fitness data
     * @param params Task parameters (unused)
     */
    void handle_fitness_task(void* params);

    /**
     * @brief Checks if the current time represents a new day compared to the last time
     * @param currentTime Current time in seconds since epoch
     * @param lastTime Previous time to compare against
     * @return true if it's a new day, false otherwise
     */
    bool is_new_day(time_t currentTime, time_t lastTime);

    /**
     * @brief Checks if the current time represents a new hour compared to the last time
     * @param currentTime Current time in seconds since epoch
     * @param lastTime Previous time to compare against
     * @return true if it's a new hour, false otherwise
     */
    bool is_new_hour(time_t currentTime, time_t lastTime);

    /**
     * @brief Handles the processing and storage of daily step counts
     * @param currentSteps Number of steps to add to the daily total
     */
    void handle_daily_steps(int currentSteps);

    /**
     * @brief Handles the processing and storage of hourly step counts
     * @param currentSteps Number of steps to add to the hourly total
     */
    void handle_hourly_steps(int currentSteps);
}