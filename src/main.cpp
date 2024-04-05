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

    appManager.registerApp("Clock", clockApp);
    appManager.registerApp("Alarm", alarmApp);
    appManager.registerApp("StopWatch", stopWatchApp);

    appManager.launchApp("Clock");

    // Create tasks for rendering and time management
    xTaskCreate(&UIManager::renderTask, "Rendering Task", 4096, &uiManager, 2, NULL);
    xTaskCreate(&TimeManager::timeTask, "Timing Task", 4096, &timeManager, 5, NULL);
    xTaskCreate(&InputManager::inputTask, "Input Task", 4096, &inputManager, 5, NULL);

    // fileManager.writeData("Alarm", "alarms.txt", "alarms=12:01");
    // std::string alarms = fileManager.readData("Alarm", "alarms.txt");
    // ESP_LOGI("Main", "File input: %s", alarms.c_str());

    // TEST cases for adding input events
    inputManager.addListener([](InputEvent event) {
        switch (event) {
            case InputEvent::JOYSTICK_UP:
                ESP_LOGI("InputManager", "Joystick moved up");
                appManager.launchNextApp();
                break;
            case InputEvent::JOYSTICK_DOWN:
                ESP_LOGI("InputManager", "Joystick moved down");
                appManager.launchPreviousApp();
                break;
            case InputEvent::BUTTON_CLICK:
                ESP_LOGI("InputManager", "Button clicked");
                break;
        }
    });
}