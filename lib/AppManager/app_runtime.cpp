#include "app_runtime.h"


extern "C" {
#define TAG "LUA_IO"

#define lua_writestring(s, l) ESP_LOGI(TAG, "%.*s", (int)(l), (s))
#define lua_writeline() ESP_LOGI(TAG, "\n")

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "luavgl.h"
}

#include "esp_log.h"
#include "FileManager.h"
#include "LVGLMutex.h"
#include <string>
#include <cstring>
#include <time.h>
#include "app_screen.h"


// used to gracefully close the lua state, can't be called directly due to click events being within lvgl tick call
static bool shouldClose = false;
static bool appOpen = false;
static lua_State *L;
static AppConfig* appConfig;
static char* luaCode;

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

    // Add custom package path to search in app's directory
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    std::string currentPath = lua_tostring(L, -1);
    
    // Construct path to app's directory (support local modules)
    std::string appPath = std::string(appConfig->entry_point);
    size_t lastSlash = appPath.find_last_of("/");
    if (lastSlash != std::string::npos) {
        appPath = "/spiffs/" + appPath.substr(0, lastSlash + 1);
    }
    std::string newPath = appPath + "?.lua;" + currentPath;
    lua_pushstring(L, newPath.c_str());
    lua_setfield(L, -3, "path");
    
    // Pop package.path and package
    lua_pop(L, 2);

    lua_register(L, "exit", luaClose);

    lua_pushcfunction(L, [](lua_State *L) -> int {
        if (appConfig && !appConfig->name.empty()) {
            lua_pushstring(L, appConfig->name.c_str());
        } else {
            lua_pushnil(L);
        }
        return 1;
    });
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

    // Append restriction logic to io.open
    lua_getglobal(L, "io");
    lua_getfield(L, -1, "open");
    lua_pushcclosure(L, [](lua_State *L) -> int {
        std::string appName = "";
        if (appConfig == nullptr) {
            appName = "lost";
        }

        const char* path = luaL_checkstring(L, 1);
        const char* mode = luaL_checkstring(L, 2);

        std::string fullPath = "/spiffs/data/" + appConfig->name + "/" + path;

        // Call the original io.open
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_pushstring(L, fullPath.c_str());
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

void runLuaScriptTask(void *pvParameters) {
    ESP_LOGI(TAG, "Running Lua script task");

    while (true) {
        ESP_LOGI(TAG, "LUA LOOP");
        if (shouldClose && appOpen) {
            LvglMutex::lock();
            ESP_LOGI(TAG, "Closing Lua state");
            if (L != nullptr) {
                signalLuaClose();
                lua_close(L);
            }
            if (luaCode != nullptr) {
                free(luaCode);
            }
            LvglMutex::unlock();
            shouldClose = false;
            appOpen = false;
        }

        if (!appOpen && appConfig != nullptr) {
            L = luaL_newstate();
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

        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}
