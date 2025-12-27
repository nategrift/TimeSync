extern "C" {
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
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
#include "LVGLMutex.h"
#include "app_runtime.h"


// END APPS

#define configUSE_TRACE_FACILITY 1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1

#define TAG "main"

#define traceTASK_SWITCHED_IN() {\
    UBaseType_t core = xPortGetCoreID();\
    const char* taskName = pcTaskGetName(NULL);\
    ESP_LOGI("TaskMonitor", "Core %d: %s", core, taskName);\
}

// Heap monitoring task - logs memory usage every 5 seconds
void heapMonitorTask(void* param) {
    const char* HEAP_TAG = "HeapMonitor";
    
    while (true) {
        size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        size_t min_free_heap = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
        size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_8BIT);
        size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        size_t free_dma = heap_caps_get_free_size(MALLOC_CAP_DMA);
        
        ESP_LOGI(HEAP_TAG, "=== HEAP STATUS ===");
        ESP_LOGI(HEAP_TAG, "Free: %u KB (%u bytes)", free_heap / 1024, free_heap);
        ESP_LOGI(HEAP_TAG, "Min Free Ever: %u KB (%u bytes)", min_free_heap / 1024, min_free_heap);
        ESP_LOGI(HEAP_TAG, "Total: %u KB (%u bytes)", total_heap / 1024, total_heap);
        ESP_LOGI(HEAP_TAG, "Used: %u KB (%u bytes)", (total_heap - free_heap) / 1024, total_heap - free_heap);
        ESP_LOGI(HEAP_TAG, "Internal Free: %u KB", free_internal / 1024);
        ESP_LOGI(HEAP_TAG, "DMA Free: %u KB", free_dma / 1024);
        ESP_LOGI(HEAP_TAG, "===================");
        
        vTaskDelay(pdMS_TO_TICKS(5000)); // Log every 5 seconds
    }
}

extern "C" void app_main() {
    // Log initial heap status
    ESP_LOGI(TAG, "=== STARTUP HEAP ===");
    ESP_LOGI(TAG, "Free: %u KB, Total: %u KB", 
        heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024,
        heap_caps_get_total_size(MALLOC_CAP_8BIT) / 1024);
    ESP_LOGI(TAG, "====================");

    // Initialize GraphicsDriver
    GraphicsDriver graphicsDriver;
    graphicsDriver.init();
    
    ESP_LOGI(TAG, "After GraphicsDriver init - Free heap: %u KB", 
        heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024);

    static TouchDriver touchDriver;
    if (touchDriver.init() == ESP_OK) {
        graphicsDriver.setupTouchDriver(touchDriver);
    }

    // setup GPIO for the awake manager
    AwakeManager::init();

    // VibrationDriver::init();

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

    ESP_LOGI(TAG, "After NVS/FileManager/Config init - Free heap: %u KB", 
        heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024);

    AppManager::loadAppsFromDisk();
    ESP_LOGI(TAG, "App Manager loaded %d apps", AppManager::getAppRegistry().size());
    ESP_LOGI(TAG, "After AppManager load - Free heap: %u KB", 
        heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024);
    for (const auto& app : AppManager::getAppRegistry()) {
        ESP_LOGI(TAG, "App: %s, Version: %s, Author: %s, Entry Point: %s", 
            app.name.c_str(),
            app.version.c_str(), 
            app.author.c_str(),
            app.entry_point.c_str());
    }

    // Create tasks for time management
    xTaskCreatePinnedToCore(&TimeManager::timeTask, "Timing Task", 4096, nullptr, 5, NULL, 1);

    ESP_LOGI(TAG, "Initializing GPIO");
    // Initialize GPIO
    gpio_set_direction(EXAMPLE_PIN_NUM_LCD_RST, GPIO_MODE_OUTPUT);

    GraphicsDriver::init_backlight_pwm();
    int brightness = ConfigManager::getConfigInt("General", "Brightness");
    GraphicsDriver::set_backlight_brightness(brightness);
    lv_display_set_rotation(NULL, LV_DISPLAY_ROTATION_90);

    // init_buzzer();

    // Reduced from 8000 to 4096 - events task doesn't need that much stack
    xTaskCreatePinnedToCore(&TimeEventsManager::checkExpiringEventsTask, "checkExpiringEventsTask", 4096, NULL, 5, NULL, 1);

    // Auto-connect to WiFi if enabled and SSID is configured
    // if (ConfigManager::getConfigInt("Network", "Enabled")) {
    //     std::string ssid = ConfigManager::getConfigString("Network", "SSID");
    //     if (!ssid.empty()) {
    //         ESP_LOGI(TAG, "Auto-connecting to WiFi: %s", ssid.c_str());
    //         WifiManager::turnOn();
    //         WifiManager::connect();
    //     }
    // }

    // vTaskDelay(pdMS_TO_TICKS(100));
    // MotionDriver::init();
    // vTaskDelay(pdMS_TO_TICKS(100));
    // MotionDriver::enableGyroAndAcc();
    // vTaskDelay(pdMS_TO_TICKS(100));
    // ret = MotionDriver::enablePedometer();
    // if (ret != ESP_OK) ESP_LOGE("MotionDebug", "can't enable pedometer");

    // char* data = file_manager_read_data("fitness", "hourly_steps.txt");
    // if (data) {
    //     ESP_LOGI(TAG, "Fitness Hourly Data: %s", data);
    //     free(data);
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

    ESP_LOGI(TAG, "After Lua task creation - Free heap: %u KB", 
        heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024);

    // Start heap monitoring task (logs every 5 seconds)
    xTaskCreatePinnedToCore(
        heapMonitorTask,
        "HeapMonitor",
        4096,  // Increased stack for ESP_LOGI formatting
        NULL,
        1,  // Low priority
        NULL,
        1
    );

    AppManager::launchApp("Clock");
    
    ESP_LOGI(TAG, "=== INIT COMPLETE ===");
    ESP_LOGI(TAG, "Free heap: %u KB, Min ever: %u KB", 
        heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024,
        heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT) / 1024);
}