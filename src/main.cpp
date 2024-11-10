extern "C" {
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "sdkconfig.h"

}

#include "buzzer_driver.h"
#include "TimeManager.h"
#include "InputManager.h"
#include "FileManager.h"
#include "AwakeManager.h"
#include "BatteryManager.h"
#include "ConfigManager.h"
#include "TimeEventsManager.h"
#include "NotificationManager.h"
#include "WifiManager.h"
#include "WifiDebug.h"
// #include "MotionDebug.h"
#include "GraphicsDriver.h"
#include "TouchDriver.h"
// #include "MotionDriver.h"
#include "VibrationDriver.h"

#include <string>


#include "lvgl.h"
#include "nvs_flash.h"


#include "AppManager.h"
// START APPS

// #include "Alarm.h"
#include "Clock.h"
#include "Stopwatch.h"
#include "Timer.h"
#include "Settings.h"
#include "AppSelector.h"

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



// void motionTask(void *pvParameters) {
//     // MotionDriver *motionDriver = static_cast<MotionDriver *>(pvParameters);
    
//     uint32_t stepCount = 0;

//     while (true) {
//         motionDriver->readStepCount(stepCount);

//         ESP_LOGI(TAG, "Step Count %d", (int)stepCount);
//         bool isPedometerRunning;
//         esp_err_t ret = motionDriver->isPedometerRunning(isPedometerRunning);
//         if (ret == ESP_OK) {
//             ESP_LOGI(TAG, "Pedometer is %s", isPedometerRunning ? "running" : "stopped");
//         } else {
//             ESP_LOGE(TAG, "Failed to check pedometer status");
//         }


//         // Wait for 2 seconds
//         vTaskDelay(pdMS_TO_TICKS(2000));
//     }
// }


// void wifiStatusTask(void *pvParameters) {
//     while (true) {
//         if (WifiManager::isConnected()) {
//             ESP_LOGI(TAG, "WiFi is connected");
//             ESP_LOGI(TAG, "Signal strength: %d dBm", WifiManager::getSignalStrength());
//             ESP_LOGI(TAG, "Local IP: %s", WifiManager::getIpAddress().c_str());
//         } else {
//             ESP_LOGI(TAG, "WiFi is not connected");
//         }
//         vTaskDelay(pdMS_TO_TICKS(5000)); // Wait for 5 seconds
//     }
// }

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
    VibrationDriver::init();

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize the ConfigManager with the path to the configuration file
    static FileManager fileManager;
    // ESP_LOGI(TAG, "Reset button held. Resetting configuration.");
    // fileManager.writeData("ConfigManager", "config.txt", "");
    fileManager.writeData("TimeEvents", "events.csv", ""); 
    ConfigManager::init();
    TimeManager::init();

    static InputManager inputManager(touchDriver);
    static BatteryManager batteryManager;

    static AppManager appManager(touchDriver, fileManager, inputManager, batteryManager);

    Clock* clockApp = new Clock(appManager);
    // Alarm* alarmApp = new Alarm(appManager);
    Stopwatch* stopWatchApp = new Stopwatch(appManager);
    Timer* timerApp = new Timer(appManager);
    Settings* settingsApp = new Settings(appManager);
    WifiDebug* wifiDebugApp = new WifiDebug(appManager);
    // MotionDebug* motionDebugApp = new MotionDebug(appManager);
    // Not selectable app
    AppSelector* appSelector = new AppSelector(appManager);

    appManager.registerApp(clockApp);
    // appManager.registerApp("Alarm", alarmApp);
    appManager.registerApp(stopWatchApp);
    appManager.registerApp(timerApp);
    appManager.registerApp(settingsApp);
    appManager.registerApp(appSelector);

    // appManager.registerApp(wifiDebugApp);
    // appManager.registerApp(motionDebugApp);

    appManager.launchApp(clockApp->getAppName());

    // Create tasks for time management
    xTaskCreatePinnedToCore(&TimeManager::timeTask, "Timing Task", 4096, nullptr, 5, NULL, 0);

    ESP_LOGI(TAG, "Initializing GPIO");
    // Initialize GPIO
    gpio_set_direction(EXAMPLE_PIN_NUM_LCD_RST, GPIO_MODE_OUTPUT);

    GraphicsDriver::init_backlight_pwm();

    int brightness = ConfigManager::getConfigInt("General", "Brightness");
    GraphicsDriver::set_backlight_brightness(brightness);

    lv_disp_set_rotation(NULL, LV_DISP_ROT_90);


    init_buzzer();

    xTaskCreate(&TimeEventsManager::checkExpiringEventsTask, "checkExpiringEventsTask", 8000, NULL, 5, NULL);

    // vTaskDelay(pdMS_TO_TICKS(4000));
    // MotionDriver motionDriver;
    // vTaskDelay(pdMS_TO_TICKS(150));
    // motionDriver.init();
    // vTaskDelay(pdMS_TO_TICKS(150));
    // motionDriver.enableGyroAndAcc();
    // vTaskDelay(pdMS_TO_TICKS(100));
    // ret = motionDriver.enablePedometer();
    // if (ret != ESP_OK) ESP_LOGI(TAG, "can;t enable petometer");;

    // xTaskCreate(motionTask, "Motion Task", 4048, &motionDriver, 5, nullptr);

    if (ConfigManager::getConfigInt("Network", "Enabled")) {
        WifiManager::turnOn();
        WifiManager::connect();
    }


    // xTaskCreate(wifiStatusTask, "WiFi Status Task", 4096, NULL, 5, NULL);

    // Initialize TimeEventsManager
    TimeEventsManager::init();

}