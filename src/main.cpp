extern "C" {
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "sdkconfig.h"
// #include "HD44780.h"
}

#include "UIManager.h"
#include "TimeManager.h"
#include "TextComponent.h"
#include "InputManager.h"
#include "FileManager.h"
#include "GraphicsDriver.h"
#include "TouchDriver.h"

#include <string>


#include "AppManager.h"
// START APPS

#include "Alarm.h"
#include "Clock.h"
#include "Stopwatch.h"
#include "AppSelector.h"
#include "lvgl.h"
// END APPS

// Define GPIO pins for the joystick and button
#define JOYSTICK_CHANNEL ADC2_CHANNEL_4
#define BUTTON_PIN GPIO_NUM_16

#define configUSE_TRACE_FACILITY 1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1

#define TAG "main"


#define traceTASK_SWITCHED_IN() {\
    UBaseType_t core = xPortGetCoreID();\
    const char* taskName = pcTaskGetName(NULL);\
    ESP_LOGI("TaskMonitor", "Core %d: %s", core, taskName);\
}

extern "C" void app_main() {
    static UIManager uiManager;
    static TimeManager timeManager;
    static InputManager inputManager(JOYSTICK_CHANNEL, BUTTON_PIN);
    static FileManager fileManager;

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

    appManager.launchApp("Clock");

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
        // continue to other listeners
        return false;
    });
    ESP_LOGI(TAG, "Initializing GPIO");

    // Initialize GPIO
    gpio_set_direction(LCD_BL_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(EXAMPLE_PIN_NUM_LCD_RST, GPIO_MODE_OUTPUT);

    // Turn on the backlight
    ESP_LOGI(TAG, "Turning on the backlight");
    gpio_set_level(LCD_BL_GPIO, 1);

    // Initialize GraphicsDriver
    GraphicsDriver graphicsDriver;
    graphicsDriver.init();

    // Add text to the center of the screen
    graphicsDriver.addTextToCenter("Hello World.");
}