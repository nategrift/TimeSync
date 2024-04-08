extern "C" {
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "sdkconfig.h"
#include "HD44780.h"
}

#include "UIManager.h"
#include "TimeManager.h"
#include "TextComponent.h"
#include "InputManager.h"
#include "FileManager.h"

#include <string>


#include "AppManager.h"
// START APPS

#include "Alarm.h"
#include "Clock.h"
#include "Stopwatch.h"
#include "AppSelector.h"

// END APPS

// Define GPIO pins for the joystick and button
#define JOYSTICK_CHANNEL ADC1_CHANNEL_7
#define BUTTON_PIN GPIO_NUM_12

#define configUSE_TRACE_FACILITY 1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1


#define traceTASK_SWITCHED_IN() {\
    UBaseType_t core = xPortGetCoreID();\
    const char* taskName = pcTaskGetName(NULL);\
    ESP_LOGI("TaskMonitor", "Core %d: %s", core, taskName);\
}

extern "C" void app_main() {
    static UIManager uiManager;
    static TimeManager timeManager;
    static InputManager inputManager(JOYSTICK_CHANNEL, BUTTON_PIN);
    FileManager fileManager;

    static AppManager appManager(uiManager, inputManager, timeManager, fileManager);

    Clock* clockApp = new Clock(appManager);
    Alarm* alarmApp = new Alarm(appManager);
    Stopwatch* stopWatchApp = new Stopwatch(appManager);

    // Not selectable app
    AppSelector* appSelector = new AppSelector(appManager);

    appManager.registerApp("Clock", clockApp);
    appManager.registerApp("Alarm", alarmApp);
    appManager.registerApp("StopWatch", stopWatchApp);
    appManager.registerApp("AppSelector", appSelector);

    appManager.launchApp("AppSelector");

    // Create tasks for rendering and time management
    xTaskCreate(&UIManager::renderTask, "Rendering Task", 4096, &uiManager, 5, NULL);
    xTaskCreate(&TimeManager::timeTask, "Timing Task", 4096, &timeManager, 5, NULL);
    xTaskCreate(&InputManager::inputTask, "Input Task", 4096, &inputManager, 4, NULL);

    inputManager.addListener([&](InputEvent event) {
    switch (event) {
        case InputEvent::JOYSTICK_UP:
            ESP_LOGI("InputManager", "Joystick moved up");
            break;
        case InputEvent::JOYSTICK_DOWN:
            ESP_LOGI("InputManager", "Joystick moved down");
            break;
        case InputEvent::BUTTON_PRESS:
            ESP_LOGI("InputManager", "Button short pressed");
            // Handle short press
            break;
        case InputEvent::BUTTON_LONG_PRESS:
            ESP_LOGI("InputManager", "Button long pressed");
            // Handle long press, e.g., launch AppSelector
            appManager.launchApp("AppSelector");
            break;
    }
});

}