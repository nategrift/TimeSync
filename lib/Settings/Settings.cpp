#include "Settings.h"

#include "esp_log.h"
#include "app_screen.h"
#include <functional>
#include <string>
#include "settings_list.h"
#include "ConfigManager.h"
#include "GraphicsDriver.h"
#include "TimeManager.h"


static const char* TAG = "Settings";

Settings::Settings(AppManager& manager)
    : IApp("Settings"), // Initialize the app name
      screenObj(NULL),
      appManager(manager) {}

Settings::~Settings() {

}

ReadCallback createIntReadCallback(const std::string& group, const std::string& key) {
    return [group, key]() -> std::string {
        int value = ConfigManager::getConfigInt(group, key);
        return std::to_string(value);
    };
}

WriteCallback createIntWriteCallback(const std::string& group, const std::string& key) {
    return [group, key](const std::string& value) {
        int intValue = std::stoi(value);
        ConfigManager::setConfigInt(group, key, intValue);
    };
}

ReadCallback createStrReadCallback(const std::string& group, const std::string& key) {
    return [group, key]() -> std::string {
        return ConfigManager::getConfigString(group, key);
    };
}

WriteCallback createStrWriteCallback(const std::string& group, const std::string& key) {
    return [group, key](const std::string& value) {
        ConfigManager::setConfigString(group, key, value);
    };
}

ReadCallback createTimeReadCallback(const std::string& group, const std::string& key) {
    return [group, key]() -> std::string {
        return ConfigManager::getConfigString(group, key);
    };
}

WriteCallback createTimeWriteCallback(const std::string& group, const std::string& key) {
    return [group, key](const std::string& value) {
        int hour, minute, second;
        sscanf(value.c_str(), "%d:%d:%d",
           &hour,
           &minute,
           &second);
        
        TimeManager::setTime(hour, minute, second);
    };
}

ReadCallback createDateReadCallback(const std::string& group, const std::string& key) {
    return [group, key]() -> std::string {
        return ConfigManager::getConfigString(group, key);
    };
}

WriteCallback createDateWriteCallback(const std::string& group, const std::string& key) {
    return [group, key](const std::string& value) {
        int year, month, day;
        sscanf(value.c_str(), "%d-%d-%d",
           &year,
           &month,
           &day);
        
        TimeManager::setDate(year, month, day);
    };
}



static std::vector<Setting> generalSettings = {
    {"Screen Timeout", createIntReadCallback("General", "ScreenTimeout"), createIntWriteCallback("General", "ScreenTimeout"), SettingType::INT, 
        "5\n10\n15\n20\n25\n30\n35\n40\n45\n50\n55\n60\n65\n70\n75\n80\n85\n90\n95"},
    {"Clock Time", createTimeReadCallback("General", "Time"), createTimeWriteCallback("General", "Time"), SettingType::TIME, nullptr},
    {"Clock Date", createDateReadCallback("General", "Date"), createDateWriteCallback("General", "Date"), SettingType::DATE, nullptr},
    {"Brightness", 
    []() -> std::string {
        return std::to_string(ConfigManager::getConfigInt("General", "Brightness"));
    }, 
    [](const std::string& value) {
        int brightness = std::stoi(value);
        GraphicsDriver::set_backlight_brightness(brightness);
        ConfigManager::setConfigInt("General", "Brightness", brightness);
    }, 
    SettingType::INT, "1\n2\n3\n4\n5\n6\n7\n8\n9\n10"}
};

void Settings::launch() {
    screenObj = get_app_container(appManager);

    lv_obj_t *tv = lv_tileview_create(screenObj);
    lv_obj_set_scrollbar_mode(tv, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(tv, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_add_flag(tv, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_radius(tv, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_pad_all(tv, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(tv, 0, LV_PART_MAIN);
    

    // General settings tile
    lv_obj_t * generalTile = lv_tileview_add_tile(tv, 0, 0, LV_DIR_RIGHT);
    lv_obj_set_style_bg_color(generalTile, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_pad_all(generalTile, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(generalTile, 0, LV_PART_MAIN);
    create_settings_flex(generalTile, generalSettings, appManager);

    // Display settings tile
    lv_obj_t * displayTile = lv_tileview_add_tile(tv, 1, 0, LV_DIR_LEFT);
    lv_obj_set_style_bg_color(displayTile, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_pad_all(displayTile, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(displayTile, 0, LV_PART_MAIN);
    std::vector<Setting> displaySettings = {
        // {"Screen Timeout", readScreenTimeout, writeScreenTimeout, SettingType::INT}
    };
    create_settings_flex(displayTile, displaySettings, appManager);
}

void Settings::close() {
    if (screenObj) {
        lv_obj_del(screenObj);
        screenObj = NULL;
    }
}


void Settings::backgroundActivity() {
    // currently nothing
}
