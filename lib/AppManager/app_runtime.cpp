#include "app_runtime.h"


extern "C" {
#define TAG "LUA_IO"

#define lua_writestring(s, l) ESP_LOGI(TAG, "%.*s", (int)(l), (s))
#define lua_writeline() ESP_LOGI(TAG, "\n")

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "luavgl.h"
#include "lvgl.h"
}

#include "esp_log.h"
#include "esp_system.h"
#include "FileManager.h"
#include "LVGLMutex.h"
#include <string>
#include <cstring>
#include <time.h>
#include <sys/time.h>
#include "app_screen.h"
#include "TimeManager.h"
#include "GraphicsDriver.h"
#include "ConfigManager.h"
#include "NotificationManager.h"
#include "TimeEventsManager.h"
#include "VibrationDriver.h"
#include "buzzer_driver.h"
#include "WifiManager.h"
#include "BatteryManager.h"
#include "esp_heap_caps.h"

static BatteryManager batteryManager;

// Custom Lua allocator that uses PSRAM
static void* lua_psram_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    (void)ud;
    (void)osize;
    
    if (nsize == 0) {
        // Free memory
        heap_caps_free(ptr);
        return NULL;
    }
    
    if (ptr == NULL) {
        // Allocate new memory in PSRAM
        return heap_caps_malloc(nsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    
    // Reallocate - try PSRAM first
    void* new_ptr = heap_caps_realloc(ptr, nsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (new_ptr == NULL) {
        // Fallback to any available memory
        new_ptr = heap_caps_realloc(ptr, nsize, MALLOC_CAP_8BIT);
    }
    return new_ptr;
}

// used to gracefully close the lua state, can't be called directly due to click events being within lvgl tick call
static bool shouldClose = false;
static bool appOpen = false;
static lua_State *L = nullptr;
static AppConfig* appConfig;
static char* luaCode;

// Flag to track if notification module is loaded
static bool notificationModuleLoaded = false;

// Pending notification queue (for thread-safe notification from other tasks)
#include <queue>
#include "freertos/semphr.h"

struct PendingNotification {
    int8_t id;
    std::string title;
    std::string message;
    bool important;
};

static std::queue<PendingNotification> pendingNotifications;
static SemaphoreHandle_t notificationQueueMutex = nullptr;

// Forward declaration
static bool showLuaNotificationInternal(int8_t id, const char* title, const char* message, bool important);

void initNotificationQueue() {
    if (notificationQueueMutex == nullptr) {
        notificationQueueMutex = xSemaphoreCreateMutex();
    }
}

void queueNotification(int8_t id, const char* title, const char* message, bool important) {
    if (notificationQueueMutex == nullptr) {
        initNotificationQueue();
    }
    
    xSemaphoreTake(notificationQueueMutex, portMAX_DELAY);
    PendingNotification notif;
    notif.id = id;
    notif.title = title;
    notif.message = message;
    notif.important = important;
    pendingNotifications.push(notif);
    xSemaphoreGive(notificationQueueMutex);
    
    ESP_LOGI(TAG, "Queued notification: ID=%d, Title='%s'", id, title);
}

void processPendingNotifications() {
    if (notificationQueueMutex == nullptr || L == nullptr || !appOpen) {
        return;
    }
    
    // Check if there are any pending notifications without holding the lock for long
    xSemaphoreTake(notificationQueueMutex, portMAX_DELAY);
    if (pendingNotifications.empty()) {
        xSemaphoreGive(notificationQueueMutex);
        return;
    }
    
    // Get the next notification
    PendingNotification notif = pendingNotifications.front();
    pendingNotifications.pop();
    xSemaphoreGive(notificationQueueMutex);
    
    // Wait for any pending LVGL operations to complete before creating new UI
    // This helps prevent SPI queue overflow
    
    // Now we're in the Lua task context, safe to call Lua
    // The Lua notification code handles LVGL locking internally
    bool shown = showLuaNotificationInternal(notif.id, notif.title.c_str(), notif.message.c_str(), notif.important);
    
    // Start vibration and sound AFTER the UI is shown
    // if (shown) {
    //     VibrationDriver::incrementalVibration(30000);
    //     incremental_buzz_pattern(30000);
    // }
    
    // Give LVGL time to process the new UI before continuing
    vTaskDelay(pdMS_TO_TICKS(100));
}

bool appRunning() {
    return appConfig != nullptr;
}

void openLuaApp(AppConfig *newAppConfig) {
    appConfig = newAppConfig;
}

void closeLuaApp() {
    appConfig = nullptr;
    shouldClose = true;
}

int luaClose(lua_State *L) {
    ESP_LOGI(TAG, "Lua state is closing from within Lua script");
    closeLuaApp();
    return 0; 
}

static lv_obj_t* error_label = nullptr;

void showLuaError(const char* errorMsg) {

    // Remove previous error screen if it exists
    if (error_label) {
        lv_obj_del(lv_obj_get_parent(error_label));
        error_label = nullptr;
    }

    // Create a full-screen container for the error
    lv_obj_t* error_screen = lv_obj_create(nullptr); // nullptr creates a new screen
    lv_obj_set_size(error_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(error_screen, lv_color_hex(0x000000), 0); // black background

    // Create the error label in the center
    error_label = lv_label_create(error_screen);
    lv_label_set_text(error_label, errorMsg);
    lv_obj_set_style_text_color(error_label, lv_color_hex(0xFF0000), 0); // red text
    lv_obj_align(error_label, LV_ALIGN_CENTER, 0, 0);

    // Create the "Settings" button at the bottom center
    lv_obj_t* settings_btn = lv_button_create(error_screen);
    lv_obj_set_width(settings_btn, 120);
    lv_obj_align(settings_btn, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_t* btn_label = lv_label_create(settings_btn);
    lv_label_set_text(btn_label, "Settings");
    lv_obj_center(btn_label);

    lv_obj_add_event_cb(settings_btn, [](lv_event_t* e) {
        AppManager::launchApp("Settings");
    }, LV_EVENT_CLICKED, nullptr);

    // Load the error screen
    lv_scr_load(error_screen);
}

void configureLuaState(lua_State *L) {
    luaL_openlibs(L);
    luaL_requiref(L, "lvgl", [](lua_State *L) -> int {
        lv_obj_t *app_root = get_app_container();
        return luaopen_lvgl(L, app_root);
    }, 1);
    lua_pop(L, 1);

    // Add custom package paths for app directory and shared modules
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    std::string currentPath = lua_tostring(L, -1);
    
    // Construct path to app's directory (support local modules)
    std::string appPath = std::string(appConfig->entry_point);
    size_t lastSlash = appPath.find_last_of("/");
    if (lastSlash != std::string::npos) {
        appPath = "/spiffs/" + appPath.substr(0, lastSlash + 1);
    }
    
    // Shared modules path (for ts_ui and other shared libraries)
    std::string sharedPath = "/spiffs/shared/";
    
    // Build new path: app directory -> shared modules -> original path
    std::string newPath = appPath + "?.lua;" +
                          sharedPath + "?.lua;" +
                          sharedPath + "?/init.lua;" +
                          currentPath;
    lua_pushstring(L, newPath.c_str());
    lua_setfield(L, -3, "path");
    
    // Pop package.path and package
    lua_pop(L, 2);

    lua_register(L, "exit", luaClose);

    // currentApp table with app info
    lua_newtable(L);
    
    // currentApp.name
    lua_pushcfunction(L, [](lua_State *L) -> int {
        if (appConfig && !appConfig->name.empty()) {
            lua_pushstring(L, appConfig->name.c_str());
        } else {
            lua_pushnil(L);
        }
        return 1;
    });
    lua_setfield(L, -2, "name");
    
    // currentApp.primaryColor - returns hex color like "#E9444C"
    lua_pushcfunction(L, [](lua_State *L) -> int {
        if (appConfig && !appConfig->primary_color.empty()) {
            std::string color = "#" + appConfig->primary_color;
            lua_pushstring(L, color.c_str());
        } else {
            lua_pushstring(L, "#E9444C");  // Default fallback
        }
        return 1;
    });
    lua_setfield(L, -2, "primaryColor");
    
    lua_setglobal(L, "currentApp");

    lua_pushcfunction(L, [](lua_State *L) {
        const char* appName = luaL_checkstring(L, 1);
        if (appName) {
            AppManager::launchApp(appName);
        }
        return 0;
    });    
    lua_setglobal(L, "openApp");

    auto lvgl_unlock = [](lua_State *L) -> int {
        LvglMutex::unlock();
        return 0;
    };
    lua_pushcfunction(L, lvgl_unlock);
    lua_setglobal(L, "LVGL_unlock");

    auto lvgl_lock = [](lua_State *L) -> int {
        LvglMutex::lock();
        return 0;
    };
    lua_pushcfunction(L, lvgl_lock);
    lua_setglobal(L, "LVGL_lock");

    // tick() -> milliseconds since boot (for stopwatch/timing)
    lua_pushcfunction(L, [](lua_State *L) -> int {
        uint32_t t = lv_tick_get();
        lua_pushnumber(L, static_cast<double>(t));
        return 1;
    });
    lua_setglobal(L, "tick");

    // Config module for system settings
    lua_newtable(L);
    
    // config.getTime() -> {hour, minute, second}
    lua_pushcfunction(L, [](lua_State *L) -> int {
        tm timeinfo = TimeManager::getLocalTimeInfo();
        lua_newtable(L);
        lua_pushinteger(L, timeinfo.tm_hour);
        lua_setfield(L, -2, "hour");
        lua_pushinteger(L, timeinfo.tm_min);
        lua_setfield(L, -2, "minute");
        lua_pushinteger(L, timeinfo.tm_sec);
        lua_setfield(L, -2, "second");
        return 1;
    });
    lua_setfield(L, -2, "getTime");
    
    // config.getDate() -> {year, month, day}
    lua_pushcfunction(L, [](lua_State *L) -> int {
        tm timeinfo = TimeManager::getLocalTimeInfo();
        lua_newtable(L);
        lua_pushinteger(L, timeinfo.tm_year + 1900);  // tm_year is years since 1900
        lua_setfield(L, -2, "year");
        lua_pushinteger(L, timeinfo.tm_mon + 1);      // tm_mon is 0-based
        lua_setfield(L, -2, "month");
        lua_pushinteger(L, timeinfo.tm_mday);
        lua_setfield(L, -2, "day");
        return 1;
    });
    lua_setfield(L, -2, "getDate");
    
    // config.setTime(hour, minute, second)
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int hour = luaL_checkinteger(L, 1);
        int minute = luaL_checkinteger(L, 2);
        int second = luaL_optinteger(L, 3, 0);
        TimeManager::setTime(hour, minute, second);
        ESP_LOGI(TAG, "Set time: %02d:%02d:%02d", hour, minute, second);
        return 0;
    });
    lua_setfield(L, -2, "setTime");
    
    // config.setDate(year, month, day)
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int year = luaL_checkinteger(L, 1);
        int month = luaL_checkinteger(L, 2);
        int day = luaL_checkinteger(L, 3);
        TimeManager::setDate(year, month, day);
        ESP_LOGI(TAG, "Set date: %04d-%02d-%02d", year, month, day);
        return 0;
    });
    lua_setfield(L, -2, "setDate");
    
    lua_setglobal(L, "config");

    // Watch module for device control
    lua_newtable(L);
    
    // watch.setBrightness(value) - value 1-10
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int brightness = luaL_checkinteger(L, 1);
        if (brightness < 1) brightness = 1;
        if (brightness > 10) brightness = 10;
        GraphicsDriver::set_backlight_brightness(brightness);
        ConfigManager::setConfigInt("General", "Brightness", brightness);
        ESP_LOGI(TAG, "Set brightness: %d", brightness);
        return 0;
    });
    lua_setfield(L, -2, "setBrightness");
    
    // watch.getBrightness() -> number (1-10)
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int brightness = ConfigManager::getConfigInt("General", "Brightness");
        lua_pushinteger(L, brightness);
        return 1;
    });
    lua_setfield(L, -2, "getBrightness");
    
    // watch.setVolume(value) - value 1-10
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int volume = luaL_checkinteger(L, 1);
        if (volume < 1) volume = 1;
        if (volume > 10) volume = 10;
        ConfigManager::setConfigInt("General", "Volume", volume);
        ESP_LOGI(TAG, "Set volume: %d", volume);
        return 0;
    });
    lua_setfield(L, -2, "setVolume");
    
    // watch.getVolume() -> number (1-10)
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int volume = ConfigManager::getConfigInt("General", "Volume");
        lua_pushinteger(L, volume);
        return 1;
    });
    lua_setfield(L, -2, "getVolume");
    
    // watch.setMute(value) - value true/false
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int mute = lua_toboolean(L, 1) ? 1 : 0;
        ConfigManager::setConfigInt("General", "Mute", mute);
        ESP_LOGI(TAG, "Set mute: %d", mute);
        return 0;
    });
    lua_setfield(L, -2, "setMute");
    
    // watch.getMute() -> boolean
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int mute = ConfigManager::getConfigInt("General", "Mute");
        lua_pushboolean(L, mute != 0);
        return 1;
    });
    lua_setfield(L, -2, "getMute");
    
    // watch.restart() - restart the device
    lua_pushcfunction(L, [](lua_State *L) -> int {
        ESP_LOGI(TAG, "Restarting device...");
        esp_restart();
        return 0;
    });
    lua_setfield(L, -2, "restart");
    
    // watch.getHeapInfo() -> {free, largest_block}
    lua_pushcfunction(L, [](lua_State *L) -> int {
        size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
        lua_newtable(L);
        lua_pushinteger(L, free_heap);
        lua_setfield(L, -2, "free");
        lua_pushinteger(L, largest);
        lua_setfield(L, -2, "largest_block");
        return 1;
    });
    lua_setfield(L, -2, "getHeapInfo");
    
    // watch.setString(key, value) - set string config value
    lua_pushcfunction(L, [](lua_State *L) -> int {
        const char* key = luaL_checkstring(L, 1);
        const char* value = luaL_checkstring(L, 2);
        ConfigManager::setConfigString("General", key, value);
        ESP_LOGI(TAG, "Set string %s: %s", key, value);
        return 0;
    });
    lua_setfield(L, -2, "setString");
    
    // watch.getString(key, default) -> string
    lua_pushcfunction(L, [](lua_State *L) -> int {
        const char* key = luaL_checkstring(L, 1);
        const char* defaultVal = luaL_optstring(L, 2, "");
        std::string value = ConfigManager::getConfigString("General", key);
        if (value.empty()) {
            lua_pushstring(L, defaultVal);
        } else {
            lua_pushstring(L, value.c_str());
        }
        return 1;
    });
    lua_setfield(L, -2, "getString");
    
    lua_setglobal(L, "watch");

    // Notifications module for notification handling
    lua_newtable(L);
    
    // notifications.dismiss(id) - dismiss a notification by ID
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int id = luaL_checkinteger(L, 1);
        NotificationManager::dismissNotification(static_cast<int8_t>(id));
        ESP_LOGI(TAG, "Dismissed notification: %d", id);
        return 0;
    });
    lua_setfield(L, -2, "dismiss");
    
    lua_setglobal(L, "notifications");

    // TimeEvents module for timer/alarm management
    lua_newtable(L);
    
    // timeEvents.add(type, expireTime, label, appName) -> id
    // type: 0 = ALARM, 1 = TIMER
    // expireTime: Unix timestamp when the event expires
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int type = luaL_checkinteger(L, 1);
        lua_Integer expireTime = luaL_checkinteger(L, 2);
        const char* label = luaL_optstring(L, 3, "");
        const char* appName = luaL_optstring(L, 4, "");
        
        EventType eventType = (type == 0) ? EventType::ALARM : EventType::TIMER;
        int8_t id = TimeEventsManager::addTimeEvent(eventType, static_cast<time_t>(expireTime), label, appName);
        
        lua_pushinteger(L, id);
        ESP_LOGI(TAG, "Added time event: ID=%d, Type=%d", id, type);
        return 1;
    });
    lua_setfield(L, -2, "add");
    
    // timeEvents.delete(id) -> boolean
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int id = luaL_checkinteger(L, 1);
        bool success = TimeEventsManager::deleteTimeEvent(static_cast<int8_t>(id));
        lua_pushboolean(L, success);
        return 1;
    });
    lua_setfield(L, -2, "delete");
    
    // timeEvents.getById(id) -> table or nil
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int id = luaL_checkinteger(L, 1);
        TimeEvent event = TimeEventsManager::getTimeEventById(static_cast<int8_t>(id));
        
        if (event.id == -1) {
            lua_pushnil(L);
            return 1;
        }
        
        lua_newtable(L);
        lua_pushinteger(L, event.id);
        lua_setfield(L, -2, "id");
        lua_pushinteger(L, static_cast<int>(event.type));
        lua_setfield(L, -2, "type");
        lua_pushinteger(L, event.expireTime);
        lua_setfield(L, -2, "expireTime");
        lua_pushstring(L, event.label.c_str());
        lua_setfield(L, -2, "label");
        lua_pushstring(L, event.appName.c_str());
        lua_setfield(L, -2, "appName");
        return 1;
    });
    lua_setfield(L, -2, "getById");
    
    // timeEvents.getAllByType(type) -> array of tables
    // type: 0 = ALARM, 1 = TIMER
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int type = luaL_checkinteger(L, 1);
        EventType eventType = (type == 0) ? EventType::ALARM : EventType::TIMER;
        std::vector<TimeEvent> events = TimeEventsManager::getAllEventsByType(eventType);
        
        lua_newtable(L);
        int index = 1;
        for (const auto& event : events) {
            lua_newtable(L);
            lua_pushinteger(L, event.id);
            lua_setfield(L, -2, "id");
            lua_pushinteger(L, static_cast<int>(event.type));
            lua_setfield(L, -2, "type");
            lua_pushinteger(L, event.expireTime);
            lua_setfield(L, -2, "expireTime");
            lua_pushstring(L, event.label.c_str());
            lua_setfield(L, -2, "label");
            lua_pushstring(L, event.appName.c_str());
            lua_setfield(L, -2, "appName");
            lua_rawseti(L, -2, index++);
        }
        return 1;
    });
    lua_setfield(L, -2, "getAllByType");
    
    // timeEvents.clearByType(type) - clear all events of a type
    // type: 0 = ALARM, 1 = TIMER
    lua_pushcfunction(L, [](lua_State *L) -> int {
        int type = luaL_checkinteger(L, 1);
        EventType eventType = (type == 0) ? EventType::ALARM : EventType::TIMER;
        TimeEventsManager::clearAllEventsByType(eventType);
        ESP_LOGI(TAG, "Cleared all time events of type: %d", type);
        return 0;
    });
    lua_setfield(L, -2, "clearByType");
    
    // timeEvents.now() -> current Unix timestamp
    lua_pushcfunction(L, [](lua_State *L) -> int {
        time_t now = time(nullptr);
        lua_pushinteger(L, now);
        return 1;
    });
    lua_setfield(L, -2, "now");
    
    // timeEvents.nowMs() -> current time in milliseconds (for stopwatch precision)
    lua_pushcfunction(L, [](lua_State *L) -> int {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        double ms = (double)tv.tv_sec * 1000.0 + (double)(tv.tv_usec / 1000);
        lua_pushnumber(L, ms);
        return 1;
    });
    lua_setfield(L, -2, "nowMs");
    
    lua_setglobal(L, "timeEvents");

    // KV (Key-Value) storage module for per-app persistent data
    // Each app's keys are prefixed with the app name for security isolation
    lua_newtable(L);
    
    // kv.set(key, value) - set a string value
    lua_pushcfunction(L, [](lua_State *L) -> int {
        const char* key = luaL_checkstring(L, 1);
        const char* value = luaL_checkstring(L, 2);
        
        if (appConfig == nullptr || appConfig->name.empty()) {
            lua_pushboolean(L, false);
            return 1;
        }
        
        // Create prefixed key: "AppName:key" for security isolation
        std::string prefixedKey = appConfig->name + ":" + key;
        
        // NVS keys have a max length of 15 characters, so we need to hash if longer
        // Use first 8 chars of app name + ":" + first 6 chars of key if needed
        if (prefixedKey.length() > 15) {
            std::string shortApp = appConfig->name.substr(0, 8);
            std::string shortKey = std::string(key).substr(0, 6);
            prefixedKey = shortApp + ":" + shortKey;
        }
        
        // Open a separate NVS namespace for app KV data
        nvs_handle_t kvHandle;
        esp_err_t err = nvs_open("appkv", NVS_READWRITE, &kvHandle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open NVS for kv.set: %s", esp_err_to_name(err));
            lua_pushboolean(L, false);
            return 1;
        }
        
        err = nvs_set_str(kvHandle, prefixedKey.c_str(), value);
        if (err == ESP_OK) {
            err = nvs_commit(kvHandle);
        }
        nvs_close(kvHandle);
        
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set kv '%s': %s", prefixedKey.c_str(), esp_err_to_name(err));
            lua_pushboolean(L, false);
        } else {
            ESP_LOGI(TAG, "kv.set: %s = %s", prefixedKey.c_str(), value);
            lua_pushboolean(L, true);
        }
        return 1;
    });
    lua_setfield(L, -2, "set");
    
    // kv.get(key, default) -> string or default
    lua_pushcfunction(L, [](lua_State *L) -> int {
        const char* key = luaL_checkstring(L, 1);
        const char* defaultVal = luaL_optstring(L, 2, "");
        
        if (appConfig == nullptr || appConfig->name.empty()) {
            lua_pushstring(L, defaultVal);
            return 1;
        }
        
        // Create prefixed key: "AppName:key" for security isolation
        std::string prefixedKey = appConfig->name + ":" + key;
        
        // NVS keys have a max length of 15 characters
        if (prefixedKey.length() > 15) {
            std::string shortApp = appConfig->name.substr(0, 8);
            std::string shortKey = std::string(key).substr(0, 6);
            prefixedKey = shortApp + ":" + shortKey;
        }
        
        // Open NVS namespace for app KV data
        nvs_handle_t kvHandle;
        esp_err_t err = nvs_open("appkv", NVS_READONLY, &kvHandle);
        if (err != ESP_OK) {
            lua_pushstring(L, defaultVal);
            return 1;
        }
        
        // First get required size
        size_t required_size;
        err = nvs_get_str(kvHandle, prefixedKey.c_str(), nullptr, &required_size);
        if (err == ESP_OK) {
            char* value = new char[required_size];
            err = nvs_get_str(kvHandle, prefixedKey.c_str(), value, &required_size);
            nvs_close(kvHandle);
            
            if (err == ESP_OK) {
                lua_pushstring(L, value);
                delete[] value;
                return 1;
            }
            delete[] value;
        } else {
            nvs_close(kvHandle);
        }
        
        lua_pushstring(L, defaultVal);
        return 1;
    });
    lua_setfield(L, -2, "get");
    
    // kv.delete(key) -> boolean
    lua_pushcfunction(L, [](lua_State *L) -> int {
        const char* key = luaL_checkstring(L, 1);
        
        if (appConfig == nullptr || appConfig->name.empty()) {
            lua_pushboolean(L, false);
            return 1;
        }
        
        // Create prefixed key
        std::string prefixedKey = appConfig->name + ":" + key;
        if (prefixedKey.length() > 15) {
            std::string shortApp = appConfig->name.substr(0, 8);
            std::string shortKey = std::string(key).substr(0, 6);
            prefixedKey = shortApp + ":" + shortKey;
        }
        
        nvs_handle_t kvHandle;
        esp_err_t err = nvs_open("appkv", NVS_READWRITE, &kvHandle);
        if (err != ESP_OK) {
            lua_pushboolean(L, false);
            return 1;
        }
        
        err = nvs_erase_key(kvHandle, prefixedKey.c_str());
        if (err == ESP_OK) {
            nvs_commit(kvHandle);
        }
        nvs_close(kvHandle);
        
        lua_pushboolean(L, err == ESP_OK);
        return 1;
    });
    lua_setfield(L, -2, "delete");
    
    // kv.setNumber(key, value) - set a number value (stored as int64)
    lua_pushcfunction(L, [](lua_State *L) -> int {
        const char* key = luaL_checkstring(L, 1);
        lua_Number value = luaL_checknumber(L, 2);
        
        if (appConfig == nullptr || appConfig->name.empty()) {
            lua_pushboolean(L, false);
            return 1;
        }
        
        std::string prefixedKey = appConfig->name + ":" + key;
        if (prefixedKey.length() > 15) {
            std::string shortApp = appConfig->name.substr(0, 8);
            std::string shortKey = std::string(key).substr(0, 6);
            prefixedKey = shortApp + ":" + shortKey;
        }
        
        nvs_handle_t kvHandle;
        esp_err_t err = nvs_open("appkv", NVS_READWRITE, &kvHandle);
        if (err != ESP_OK) {
            lua_pushboolean(L, false);
            return 1;
        }
        
        // Store as string to preserve precision
        std::string strValue = std::to_string(static_cast<int64_t>(value));
        err = nvs_set_str(kvHandle, prefixedKey.c_str(), strValue.c_str());
        if (err == ESP_OK) {
            err = nvs_commit(kvHandle);
        }
        nvs_close(kvHandle);
        
        lua_pushboolean(L, err == ESP_OK);
        return 1;
    });
    lua_setfield(L, -2, "setNumber");
    
    // kv.getNumber(key, default) -> number
    lua_pushcfunction(L, [](lua_State *L) -> int {
        const char* key = luaL_checkstring(L, 1);
        lua_Number defaultVal = luaL_optnumber(L, 2, 0);
        
        if (appConfig == nullptr || appConfig->name.empty()) {
            lua_pushnumber(L, defaultVal);
            return 1;
        }
        
        std::string prefixedKey = appConfig->name + ":" + key;
        if (prefixedKey.length() > 15) {
            std::string shortApp = appConfig->name.substr(0, 8);
            std::string shortKey = std::string(key).substr(0, 6);
            prefixedKey = shortApp + ":" + shortKey;
        }
        
        nvs_handle_t kvHandle;
        esp_err_t err = nvs_open("appkv", NVS_READONLY, &kvHandle);
        if (err != ESP_OK) {
            lua_pushnumber(L, defaultVal);
            return 1;
        }
        
        size_t required_size;
        err = nvs_get_str(kvHandle, prefixedKey.c_str(), nullptr, &required_size);
        if (err == ESP_OK) {
            char* value = new char[required_size];
            err = nvs_get_str(kvHandle, prefixedKey.c_str(), value, &required_size);
            nvs_close(kvHandle);
            
            if (err == ESP_OK) {
                lua_pushnumber(L, std::stoll(value));
                delete[] value;
                return 1;
            }
            delete[] value;
        } else {
            nvs_close(kvHandle);
        }
        
        lua_pushnumber(L, defaultVal);
        return 1;
    });
    lua_setfield(L, -2, "getNumber");
    
    lua_setglobal(L, "kv");

    // WiFi module
    lua_newtable(L);
    
    // wifi.isOn() -> boolean
    lua_pushcfunction(L, [](lua_State *L) -> int {
        lua_pushboolean(L, WifiManager::isOn());
        return 1;
    });
    lua_setfield(L, -2, "isOn");
    
    // wifi.setEnabled(enabled) - turn WiFi on/off (async)
    lua_pushcfunction(L, [](lua_State *L) -> int {
        bool enabled = lua_toboolean(L, 1);
        
        xTaskCreatePinnedToCore([](void* p) {
            bool enable = (bool)(uintptr_t)p;
            ConfigManager::setConfigInt("Network", "Enabled", enable ? 1 : 0);
            if (enable) WifiManager::turnOn();
            else WifiManager::turnOff();
            vTaskDelete(NULL);
        }, "wifi_toggle", 4096, (void*)(uintptr_t)enabled, 5, nullptr, 1);
        
        return 0;
    });
    lua_setfield(L, -2, "setEnabled");
    
    // wifi.isEnabled() -> boolean (config setting)
    lua_pushcfunction(L, [](lua_State *L) -> int {
        lua_pushboolean(L, ConfigManager::getConfigInt("Network", "Enabled") != 0);
        return 1;
    });
    lua_setfield(L, -2, "isEnabled");
    
    // wifi.startScan()
    lua_pushcfunction(L, [](lua_State *L) -> int {
        WifiManager::startScan();
        return 0;
    });
    lua_setfield(L, -2, "startScan");
    
    // wifi.isScanInProgress() -> boolean
    lua_pushcfunction(L, [](lua_State *L) -> int {
        lua_pushboolean(L, WifiManager::isScanInProgress());
        return 1;
    });
    lua_setfield(L, -2, "isScanInProgress");
    
    // wifi.getScannedNetworks() -> array
    lua_pushcfunction(L, [](lua_State *L) -> int {
        std::vector<NetworkInfo> networks = WifiManager::getScannedNetworks();
        lua_newtable(L);
        int i = 1;
        for (const auto& n : networks) {
            lua_newtable(L);
            lua_pushstring(L, n.ssid.c_str());
            lua_setfield(L, -2, "ssid");
            lua_pushinteger(L, n.rssi);
            lua_setfield(L, -2, "rssi");
            lua_pushstring(L, n.getAuthModeString().c_str());
            lua_setfield(L, -2, "authMode");
            lua_pushstring(L, n.getSignalQuality().c_str());
            lua_setfield(L, -2, "signalQuality");
            lua_pushboolean(L, n.isOpen());
            lua_setfield(L, -2, "isOpen");
            lua_rawseti(L, -2, i++);
        }
        return 1;
    });
    lua_setfield(L, -2, "getScannedNetworks");
    
    // wifi.selectNetwork(ssid, password) - save to config and connect (async)
    lua_pushcfunction(L, [](lua_State *L) -> int {
        const char* ssid = luaL_checkstring(L, 1);
        const char* password = luaL_optstring(L, 2, "");
        
        ConfigManager::setConfigString("Network", "SSID", ssid);
        ConfigManager::setConfigString("Network", "Password", password);
        ESP_LOGI(TAG, "Network saved: %s", ssid);
        
        xTaskCreatePinnedToCore([](void* p) {
            WifiManager::connect();
            vTaskDelete(NULL);
        }, "wifi_connect", 4096, nullptr, 5, nullptr, 1);
        
        return 0;
    });
    lua_setfield(L, -2, "selectNetwork");
    
    // wifi.isConnected() -> boolean
    lua_pushcfunction(L, [](lua_State *L) -> int {
        lua_pushboolean(L, WifiManager::isConnected());
        return 1;
    });
    lua_setfield(L, -2, "isConnected");
    
    // wifi.getConnectedSSID() -> string
    lua_pushcfunction(L, [](lua_State *L) -> int {
        lua_pushstring(L, WifiManager::getConnectedSSID().c_str());
        return 1;
    });
    lua_setfield(L, -2, "getConnectedSSID");
    
    // wifi.getSignalStrength() -> number (RSSI in dBm)
    lua_pushcfunction(L, [](lua_State *L) -> int {
        lua_pushinteger(L, WifiManager::getSignalStrength());
        return 1;
    });
    lua_setfield(L, -2, "getSignalStrength");
    
    // wifi.disconnect()
    lua_pushcfunction(L, [](lua_State *L) -> int {
        WifiManager::disconnect();
        return 0;
    });
    lua_setfield(L, -2, "disconnect");
    
    lua_setglobal(L, "wifi");

    // Battery bindings
    lua_newtable(L);
    
    // battery.getLevel() -> number (0-100)
    lua_pushcfunction(L, [](lua_State *L) -> int {
        lua_pushinteger(L, batteryManager.getBatteryLevel());
        return 1;
    });
    lua_setfield(L, -2, "getLevel");
    
    // battery.isCharging() -> boolean
    lua_pushcfunction(L, [](lua_State *L) -> int {
        lua_pushboolean(L, batteryManager.getBatteryCharging());
        return 1;
    });
    lua_setfield(L, -2, "isCharging");
    
    lua_setglobal(L, "battery");

    // Append restriction logic to io.open
    // Only sandbox relative paths (for app data files)
    // Absolute paths (starting with /) are passed through unchanged (for require/package loading)
    lua_getglobal(L, "io");
    lua_getfield(L, -1, "open");
    lua_pushcclosure(L, [](lua_State *L) -> int {
        const char* path = luaL_checkstring(L, 1);
        const char* mode = luaL_optstring(L, 2, "r");

        std::string finalPath;
        
        // If path is absolute (starts with /), pass through unchanged
        // This allows require/package loading to work with absolute paths
        if (path[0] == '/') {
            finalPath = path;
        } else {
            // Relative path: sandbox to app's data directory
            if (appConfig != nullptr) {
                finalPath = "/spiffs/data/" + appConfig->name + "/" + path;
            } else {
                finalPath = std::string("/spiffs/") + path;
            }
        }

        // Call the original io.open
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_pushstring(L, finalPath.c_str());
        lua_pushstring(L, mode);
        lua_call(L, 2, 2);

        return 2; // Return file handle and error message
    }, 1);
    lua_setfield(L, -2, "open");
    lua_pop(L, 1); // Pop io table
}

void signalLuaClose() {
        lua_getglobal(L, "OnClose");
        
        // Check if onClose exists and is a function
        if (lua_isfunction(L, -1)) {
            // Call the onClose function
            if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                ESP_LOGE(TAG, "Error calling OnClose: %s", lua_tostring(L, -1));
                lua_pop(L, 1); // Pop error message
            }
        } else {
            lua_pop(L, 1); // Pop the nil value if onClose wasn't found
        }
}

void loadNotificationModule() {
    if (L == nullptr || notificationModuleLoaded) {
        return;
    }
    
    // Load the notification module to register the global _showNotification function
    if (luaL_dostring(L, "require('ts_ui.notification')") != LUA_OK) {
        ESP_LOGE(TAG, "Failed to load notification module: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return;
    }
    notificationModuleLoaded = true;
    ESP_LOGI(TAG, "Notification module loaded");
}

// Internal function - only call from Lua task context!
static bool showLuaNotificationInternal(int8_t id, const char* title, const char* message, bool important) {
    if (L == nullptr || !appOpen) {
        ESP_LOGW(TAG, "Cannot show notification: Lua state not ready");
        return false;
    }
    
    // Ensure notification module is loaded
    loadNotificationModule();
    
    if (!notificationModuleLoaded) {
        ESP_LOGW(TAG, "Notification module not loaded");
        return false;
    }
    
    // Get the global _showNotification function
    lua_getglobal(L, "_showNotification");
    
    if (!lua_isfunction(L, -1)) {
        ESP_LOGW(TAG, "_showNotification function not found");
        lua_pop(L, 1);
        return false;
    }
    
    // Push arguments: id, title, message, important
    lua_pushinteger(L, id);
    lua_pushstring(L, title);
    lua_pushstring(L, message);
    lua_pushboolean(L, important);
    
    // Call the function with 4 arguments, 0 return values
    // Note: The Lua function handles LVGL locking internally
    if (lua_pcall(L, 4, 0, 0) != LUA_OK) {
        ESP_LOGE(TAG, "Error calling _showNotification: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }
    
    ESP_LOGI(TAG, "Showed notification via Lua: ID=%d, Title='%s'", id, title);
    return true;
}

// Public function - thread-safe, queues notification for Lua task to process
bool showLuaNotification(int8_t id, const char* title, const char* message, bool important) {
    queueNotification(id, title, message, important);
    return true;
}

void runLuaScriptTask(void *pvParameters) {
    ESP_LOGI(TAG, "Running Lua script task");
    
    // Initialize the notification queue
    initNotificationQueue();

    while (true) {
        ESP_LOGI(TAG, "LUA LOOP");
        if (shouldClose && appOpen) {
            LvglMutex::lock();
            ESP_LOGI(TAG, "Closing Lua state");
            if (L != nullptr) {
                signalLuaClose();
                lua_close(L);
                L = nullptr;
            }
            if (luaCode != nullptr) {
                free(luaCode);
                luaCode = nullptr;
            }
            LvglMutex::unlock();
            shouldClose = false;
            appOpen = false;
            notificationModuleLoaded = false;
        }

        if (!appOpen && appConfig != nullptr) {
            // Use custom PSRAM allocator for Lua to save internal RAM
            L = lua_newstate(lua_psram_alloc, NULL);
            luaL_openlibs(L);  // Open standard libraries (needed since we're not using luaL_newstate)
            configureLuaState(L);
            ESP_LOGI(TAG, "Opening app: %s", appConfig->name.c_str());
            luaCode = file_manager_read_data_root(appConfig->entry_point.c_str());
            appOpen = true;

            if (luaCode != nullptr && luaL_dostring(L, luaCode) != LUA_OK) {
                const char* err = lua_tostring(L, -1);
                printf("Error: %s\n", err);
                LvglMutex::lock();
                showLuaError(err);
                LvglMutex::unlock();
            }
        }
        
        processPendingNotifications();

        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}
