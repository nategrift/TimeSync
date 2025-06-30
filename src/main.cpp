extern "C" {
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "sdkconfig.h"

}

#include "lvgl.h"

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
#include "MotionDriver.h"
#include "GraphicsDriver.h"
#include "TouchDriver.h"
#include "VibrationDriver.h"
#include "FitnessManager.h"
#include <string>



#include "nvs_flash.h"


#include "AppManager.h"
// START APPS

// #include "Alarm.h"
// #include "Clock.h"
// #include "Stopwatch.h"
// #include "Timer.h"
// #include "Settings.h"
// #include "AppSelector.h"
// #include "Fitness.h"

// Debug apps
// #include "WifiDebug.h"
// #include "MotionDebug.h"

#include "LVGLMutex.h"
#include "app_runtime.h"


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
    VibrationDriver::init();

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    file_manager_init();

    // Initialize the ConfigManager with the path to the configuration file
    // ESP_LOGI(TAG, "Reset button held. Resetting configuration.");
    // fileManager.writeData("ConfigManager", "config.txt", "");
    file_manager_write_data("TimeEvents", "events.csv", ""); 
    ConfigManager::init();
    TimeManager::init();

    static InputManager inputManager(touchDriver);
    static BatteryManager batteryManager;

    lv_indev_t *indev = lv_indev_get_act();
    ESP_LOGI(TAG, "indev: %s", indev == NULL ? "NULL" : "NOT NULL");

    AppManager::loadAppsFromDisk();
    ESP_LOGI(TAG, "App Manager loaded %d apps", AppManager::getAppRegistry().size());
    for (const auto& app : AppManager::getAppRegistry()) {
        ESP_LOGI(TAG, "App: %s, Version: %s, Author: %s, Entry Point: %s", 
            app.name.c_str(),
            app.version.c_str(), 
            app.author.c_str(),
            app.entry_point.c_str());
    }

    // Clock* clockApp = new Clock(appManager);
    // Alarm* alarmApp = new Alarm(appManager);
    // Stopwatch* stopWatchApp = new Stopwatch(appManager);
    // Timer* timerApp = new Timer(appManager);
    // Settings* settingsApp = new Settings(appManager);
    // Fitness* fitnessApp = new Fitness(appManager);
    // WifiDebug* wifiDebugApp = new WifiDebug(appManager);
    // MotionDebug* motionDebugApp = new MotionDebug(appManager);
    // Not selectable app
    // AppSelector* appSelector = new AppSelector(appManager);

    // appManager.registerApp(clockApp);
    // appManager.registerApp("Alarm", alarmApp);
    // appManager.registerApp(stopWatchApp);
    // appManager.registerApp(timerApp);
    // appManager.registerApp(settingsApp);
    // appManager.registerApp(fitnessApp);
    // appManager.registerApp(appSelector);

    // appManager.registerApp(wifiDebugApp);
    // appManager.registerApp(motionDebugApp);

    // appManager.launchApp(clockApp->getAppName());

    // Create tasks for time management
    xTaskCreatePinnedToCore(&TimeManager::timeTask, "Timing Task", 4096, nullptr, 5, NULL, 0);

    ESP_LOGI(TAG, "Initializing GPIO");
    // Initialize GPIO
    gpio_set_direction(EXAMPLE_PIN_NUM_LCD_RST, GPIO_MODE_OUTPUT);

    GraphicsDriver::init_backlight_pwm();

    int brightness = ConfigManager::getConfigInt("General", "Brightness");
    GraphicsDriver::set_backlight_brightness(brightness);

    lv_display_set_rotation(NULL, LV_DISPLAY_ROTATION_90);

    init_buzzer();

    xTaskCreatePinnedToCore(&TimeEventsManager::checkExpiringEventsTask, "checkExpiringEventsTask", 8000, NULL, 5, NULL, 0);

    if (ConfigManager::getConfigInt("Network", "Enabled")) {
        WifiManager::turnOn();
        WifiManager::connect();
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    MotionDriver::init();
    vTaskDelay(pdMS_TO_TICKS(100));
    MotionDriver::enableGyroAndAcc();
    vTaskDelay(pdMS_TO_TICKS(100));
    ret = MotionDriver::enablePedometer();
    if (ret != ESP_OK) ESP_LOGE("MotionDebug", "can't enable pedometer");

    char* data = file_manager_read_data("fitness", "hourly_steps.txt");
    if (data) {
        ESP_LOGI(TAG, "Fitness Hourly Data: %s", data);
        free(data);
    }


    // // Check if the image file exists before setting it as the source using LVGL's file system API
    // lv_fs_file_t f;
    // lv_fs_res_t res = lv_fs_open(&f, "A:/spiffs/apps/Clock/house-solid.png", LV_FS_MODE_RD);
    // if (res == LV_FS_RES_OK) {
    //     lv_fs_close(&f);
    //     lv_obj_t *img = lv_image_create(lv_screen_active());
    //     lv_image_set_src(img, "A:/spiffs/apps/Clock/house-solid.png");
    //     lv_obj_set_pos(img, 120, 120);
    //     lv_obj_t *label = lv_label_create(lv_screen_active());
    //     lv_label_set_text(label, "Image found");
    //     lv_obj_set_pos(label, 100, 120);
    //     lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_GREY), 0);
    // } else {
    //     lv_obj_t *label = lv_label_create(lv_screen_active());
    //     lv_label_set_text(label, "Image not found");
    //     lv_obj_set_pos(label, 120, 120);
    //     lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_GREY), 0); // Set text color to white
    // }
    // xTaskCreate(
    //     FitnessManager::handle_fitness_task,
    //     "fitness_task",
    //     8192,
    //     NULL,
    //     5,
    //     NULL
    // );

    TimeEventsManager::init();

    // Create a task to run the Lua scripts (the apps)
    xTaskCreatePinnedToCore(
        runLuaScriptTask,
        "LuaScriptTask", 
        8192*2,
        NULL,
        5,               
        NULL,            
        0                
    );

    AppManager::launchApp("Clock");
}