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
#include "BatteryManager.h"

#include "GraphicsDriver.h"
#include "TouchDriver.h"

#include <string>


#include "AppManager.h"
// START APPS

// #include "Alarm.h"
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
    // Initialize GraphicsDriver
    GraphicsDriver graphicsDriver;
    graphicsDriver.init();

    static TouchDriver touchDriver;
    if (touchDriver.init() == ESP_OK) {
        graphicsDriver.setupTouchDriver(touchDriver);
    }

    static UIManager uiManager;
    static FileManager fileManager;
    static TimeManager timeManager(fileManager);
    static InputManager inputManager(touchDriver);
    static BatteryManager batteryManager;

    static AppManager appManager(touchDriver, uiManager, inputManager, timeManager, fileManager, batteryManager);

    Clock* clockApp = new Clock(appManager);
    // Alarm* alarmApp = new Alarm(appManager);
    Stopwatch* stopWatchApp = new Stopwatch(appManager);

    // Not selectable app
    AppSelector* appSelector = new AppSelector(appManager);

    appManager.registerApp("Clock", clockApp);
    // appManager.registerApp("Alarm", alarmApp);
    appManager.registerApp("StopWatch", stopWatchApp);
    appManager.registerApp("AppSelector", appSelector);

    appManager.launchApp("AppSelector");

    // Create tasks for rendering and time management
    xTaskCreate(&UIManager::renderTask, "Rendering Task", 4096, &uiManager, 5, NULL);
    xTaskCreate(&TimeManager::timeTask, "Timing Task", 4096, &timeManager, 5, NULL);

    
    ESP_LOGI(TAG, "Initializing GPIO");

    // std::string exampleData("4:12:56 AM");
    // fileManager.writeData("TimeManager", "time.txt", exampleData);
    std::string data = fileManager.readData("TimeManager", "time.txt");
    ESP_LOGI(TAG, "Content: %s", data.c_str());
    

    // Initialize GPIO
    gpio_set_direction(LCD_BL_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(EXAMPLE_PIN_NUM_LCD_RST, GPIO_MODE_OUTPUT);

    // Turn on the backlight
    ESP_LOGI(TAG, "Turning on the backlight");
    gpio_set_level(LCD_BL_GPIO, 1);
}

