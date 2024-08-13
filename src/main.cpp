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
#include "AwakeManager.h"
#include "BatteryManager.h"
#include "ConfigManager.h"

#include "GraphicsDriver.h"
#include "TouchDriver.h"

#include <string>


#include "AppManager.h"
// START APPS

// #include "Alarm.h"
#include "Clock.h"
#include "Stopwatch.h"
#include "Settings.h"
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

    // setup GPIO for the awake manager
    AwakeManager::init();

    static UIManager uiManager;
    static FileManager fileManager;
    static TimeManager timeManager(fileManager);
    static InputManager inputManager(touchDriver);
    static BatteryManager batteryManager;

    static AppManager appManager(touchDriver, uiManager, fileManager, timeManager, inputManager, batteryManager);

    Clock* clockApp = new Clock(appManager);
    // Alarm* alarmApp = new Alarm(appManager);
    Stopwatch* stopWatchApp = new Stopwatch(appManager);
    Settings* settingsApp = new Settings(appManager);

    // Not selectable app
    AppSelector* appSelector = new AppSelector(appManager);

    appManager.registerApp(clockApp);
    // appManager.registerApp("Alarm", alarmApp);
    appManager.registerApp(stopWatchApp);
    appManager.registerApp(settingsApp);
    appManager.registerApp(appSelector);

    appManager.launchApp(clockApp->getAppName());

    // Create tasks for time management
    xTaskCreatePinnedToCore(&TimeManager::timeTask, "Timing Task", 4096, &timeManager, 5, NULL, 0);

    ESP_LOGI(TAG, "Initializing GPIO");
    // Initialize GPIO
    gpio_set_direction(LCD_BL_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(EXAMPLE_PIN_NUM_LCD_RST, GPIO_MODE_OUTPUT);

    // Turn on the backlight
    ESP_LOGI(TAG, "Turning on the backlight");
    gpio_set_level(LCD_BL_GPIO, 1);

    // Initialize the ConfigManager with the path to the configuration file
     ConfigManager::init(fileManager, "config.txt");

    bool stringSaved = ConfigManager::setConfigString("GroupName", "KeyName", "First Value");
    bool intSaved = ConfigManager::setConfigInt("GroupName", "IntKey", 20);


    // Retrieve configuration values
    std::string myValue = ConfigManager::getConfigString("GroupName", "KeyName");
    int myIntValue = ConfigManager::getConfigInt("GroupName", "IntKey");

    // Log the retrieved values
    ESP_LOGI(TAG, "Initial Config String: GroupName:KeyName = %s", myValue.c_str());
    ESP_LOGI(TAG, "Initial Config Int: GroupName:IntKey = %d", myIntValue);

    // Check if a configuration value exists and update it
    if (ConfigManager::hasConfigValue("GroupName", "KeyName")) {
        bool stringSaved = ConfigManager::setConfigString("GroupName", "KeyName", "NewValue");
        bool intSaved = ConfigManager::setConfigInt("GroupName", "IntKey", 42);

        // Log the update status
        ESP_LOGI(TAG, "String value updated: %s", stringSaved ? "Success" : "Failure");
        ESP_LOGI(TAG, "Int value updated: %s", intSaved ? "Success" : "Failure");
    }

    // Retrieve and log the updated values to verify
    std::string updatedValue = ConfigManager::getConfigString("GroupName", "KeyName");
    int updatedIntValue = ConfigManager::getConfigInt("GroupName", "IntKey");

    ESP_LOGI(TAG, "Updated Config String: GroupName:KeyName = %s", updatedValue.c_str());
    ESP_LOGI(TAG, "Updated Config Int: GroupName:IntKey = %d", updatedIntValue);




}

