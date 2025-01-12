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
#include "Clock.h"
// #include "Stopwatch.h"
// #include "Timer.h"
// #include "Settings.h"
#include "AppSelector.h"
// #include "Fitness.h"

// Debug apps
// #include "WifiDebug.h"
// #include "MotionDebug.h"

#include "LVGLMutex.h"


extern "C" {
    #define TAG "LUA_IO"

    #define lua_writestring(s, l) ESP_LOGI(TAG, "%.*s", (int)(l), (s))
    #define lua_writeline() ESP_LOGI(TAG, "\n")

    #include "lua.h"
    #include "lauxlib.h"
    #include "lualib.h"

    #include "luavgl.h"
    
}

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

static bool shouldClose = false;

int luaClose(lua_State *L) {
    ESP_LOGI(TAG, "Lua state is closing from within Lua script");
    shouldClose = true;
    return 0; 
}

void runLuaScriptTask(void *pvParameters) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    luaL_requiref(L, "lvgl", luaopen_lvgl, 1);
    lua_pop(L, 1);
    // add a function to close lua state
    lua_register(L, "close", luaClose);

    FileManager* fileManager = static_cast<FileManager*>(pvParameters);

    std::string luaCode = fileManager->readData("apps", "test.lua");
    ESP_LOGI(TAG, "Data: %s", luaCode.c_str());

    if (luaL_dostring(L, luaCode.c_str()) != LUA_OK) {
        printf("Error: %s\n", lua_tostring(L, -1));
    }

    while (!shouldClose) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay to prevent tight loop
    }

    LvglMutex::lock();
    lua_close(L);
    LvglMutex::unlock();

     while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay to prevent tight loop
    }
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
    // Stopwatch* stopWatchApp = new Stopwatch(appManager);
    // Timer* timerApp = new Timer(appManager);
    // Settings* settingsApp = new Settings(appManager);
    // Fitness* fitnessApp = new Fitness(appManager);
    // WifiDebug* wifiDebugApp = new WifiDebug(appManager);
    // MotionDebug* motionDebugApp = new MotionDebug(appManager);
    // Not selectable app
    AppSelector* appSelector = new AppSelector(appManager);

    appManager.registerApp(clockApp);
    // appManager.registerApp("Alarm", alarmApp);
    // appManager.registerApp(stopWatchApp);
    // appManager.registerApp(timerApp);
    // appManager.registerApp(settingsApp);
    // appManager.registerApp(fitnessApp);
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

    std::string data = FileManager::readData("fitness", "hourly_steps.txt");
    ESP_LOGI(TAG, "Fitness Hourly Data: %s", data.c_str());

    
    // xTaskCreate(
    //     FitnessManager::handle_fitness_task,
    //     "fitness_task",
    //     8192,
    //     NULL,
    //     5,
    //     NULL
    // );

    TimeEventsManager::init();

    // Create a task to run the Lua script on core 1
    xTaskCreatePinnedToCore(
        runLuaScriptTask,   // Task function
        "LuaScriptTask",    // Task name
        8192,               // Stack size
        &fileManager,       // Task parameter
        5,                  // Task priority
        NULL,               // Task handle
        0                   // Core ID (0 or 1)
    );
}