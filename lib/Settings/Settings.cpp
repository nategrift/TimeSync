#include "Settings.h"

#include "esp_log.h"
#include "app_screen.h"
#include <functional>
#include <string>
#include "settings_list.h"
#include "ConfigManager.h"
#include "GraphicsDriver.h"
#include "TimeManager.h"
#include "esp_system.h"
#include "WifiManager.h"
#include "NotificationManager.h"
#include "MotionDriver.h"
#include "FitnessManager.h"


static const char *TAG = "Settings";

static void restart_esp32()
{
    esp_restart();
}

Settings::Settings(AppManager &manager)
    : IApp("Settings"), // Initialize the app name
      screenObj(NULL),
      appManager(manager)
{

    generalSettings = {
        // Screen Timeout
        {"Screen Timeout",
         []() -> std::string
         {
             return std::to_string(ConfigManager::getConfigInt("General", "ScreenTimeout"));
         },
         [](const std::string &value)
         {
             ConfigManager::setConfigInt("General", "ScreenTimeout", std::stoi(value));
         },
         []() -> std::string
         {
             return std::to_string(ConfigManager::getConfigInt("General", "ScreenTimeout"));
         },
         SettingType::INT, "5\n10\n15\n20\n25\n30\n35\n40\n45\n50\n55\n60\n65\n70\n75\n80\n85\n90\n95"},

        // Time
        {"Clock Time",
         []() -> std::string
         {
             return ConfigManager::getConfigString("General", "Time");
         },
         [](const std::string &value)
         {
             int hour, minute, second;
             sscanf(value.c_str(), "%d:%d:%d", &hour, &minute, &second);
             TimeManager::setTime(hour, minute, second);
         },
         []() -> std::string
         {
             return ConfigManager::getConfigString("General", "Time");
         },
         SettingType::TIME, nullptr},

        // Date
        {"Clock Date",
         []() -> std::string
         {
             return ConfigManager::getConfigString("General", "Date");
         },
         [](const std::string &value)
         {
             int year, month, day;
             sscanf(value.c_str(), "%d-%d-%d", &year, &month, &day);
             TimeManager::setDate(year, month, day);
         },
         []() -> std::string
         {
             return ConfigManager::getConfigString("General", "Date");
         },
         SettingType::DATE, nullptr},
        {"Fetch Time",
         []() -> std::string
         { return ""; },
         [](const std::string &value)
         { 
            if (WifiManager::isConnected()) {
                // Add world time information
                std::string errorMsg;
                time_t time;
                if (WifiManager::fetchWorldTime(errorMsg, time)) {
                    // Convert string to time_t and set system time
                    struct timeval tv = { .tv_sec = time, .tv_usec = 0 };
                    if (settimeofday(&tv, NULL) == 0) {
                        NotificationManager::createNotification(
                            1,
                            "Time Set", 
                            "System time set successfully", 
                            []() {
                                
                            }, 
                            false
                        );
                    } else {
                        NotificationManager::createNotification(
                            2,
                            "Time Set", 
                            "Failed to set system time", 
                            []() {}, 
                            false
                        );
                    }
                } else {
                    NotificationManager::createNotification(
                        3,
                        "Time Set", 
                        "Time Error: " + errorMsg, 
                        []() {}, 
                        false
                    );
                }
            }


          },
         []() -> std::string
         { return ""; },
         SettingType::BUTTON, nullptr},

        // Brightness
        {"Brightness",
         []() -> std::string
         {
             return ConfigManager::getConfigString("General", "Brightness");
         },
         [](const std::string &value)
         {
             int brightness = std::stoi(value);
             GraphicsDriver::set_backlight_brightness(brightness);
             ConfigManager::setConfigInt("General", "Brightness", brightness);
         },
         []() -> std::string
         {
             return ConfigManager::getConfigString("General", "Brightness");
         },
         SettingType::INT, "1\n2\n3\n4\n5\n6\n7\n8\n9\n10"},
        // PedometerLevel
        {"PedometerLevel",
         []() -> std::string
         {
             return ConfigManager::getConfigString("General", "PedometerLevel");
         },
         [](const std::string &value)
         {
            int level = std::stoi(value);
            ConfigManager::setConfigInt("General", "PedometerLevel", level);
            vTaskDelay(pdMS_TO_TICKS(100));
            MotionDriver::init();
            vTaskDelay(pdMS_TO_TICKS(100));
            MotionDriver::enableGyroAndAcc();
            vTaskDelay(pdMS_TO_TICKS(100));
            MotionDriver::enablePedometer();
         },
         []() -> std::string
         {
             return ConfigManager::getConfigString("General", "PedometerLevel");
         },
         SettingType::INT, "1\n2\n3\n4\n5"},

        {"Volume",
         []() -> std::string
         {
             return std::to_string(ConfigManager::getConfigInt("General", "Volume"));
         },
         [](const std::string &value)
         {
             int volume = std::stoi(value);
             ConfigManager::setConfigInt("General", "Volume", volume);
             // TODO: Implement volume change functionality
         },
         []() -> std::string
         {
             return std::to_string(ConfigManager::getConfigInt("General", "Volume"));
         },
         SettingType::INT, "1\n2\n3\n4\n5\n6\n7\n8\n9\n10"},

        // Mute
        {"Mute",
         []() -> std::string
         {
             return ConfigManager::getConfigInt("General", "Mute") ? "1" : "0";
         },
         [](const std::string &value)
         {
             int muteValue = (value == "1") ? 1 : 0;
             ConfigManager::setConfigInt("General", "Mute", muteValue);
         },
         []() -> std::string
         {
             return ConfigManager::getConfigInt("General", "Mute") ? "On" : "Off";
         },
         SettingType::BOOL, nullptr},

        {"Reset Steps Logs",
         []() -> std::string
         { return ""; },
         [](const std::string &value)
         { 
            FileManager::writeData("fitness", "hourly_steps.txt", ""); 
            ConfigManager::setConfigInt(FitnessManager::KEY, FitnessManager::DAILY_STEPS_KEY, 0);
         },
         []() -> std::string
         { return ""; },
         SettingType::BUTTON, nullptr},

        {"Restart Device",
         []() -> std::string
         { return ""; },
         [](const std::string &value)
         { restart_esp32(); },
         []() -> std::string
         { return ""; },
         SettingType::BUTTON, nullptr},
    };

    networkSettings = {
        {"Device Name",
         []() -> std::string
         {
             return ConfigManager::getConfigString("General", "Name");
         },
         [](const std::string &value)
         {
             ConfigManager::setConfigString("General", "Name", value);
         },
         []() -> std::string
         {
             return ConfigManager::getConfigString("General", "Name");
         },
         SettingType::STRING, nullptr},
        {"Enabled",
         []() -> std::string
         {
             return ConfigManager::getConfigInt("Network", "Enabled") ? "1" : "0";
         },
         [](const std::string &value)
         {
             int enabledValue = (value == "1") ? 1 : 0;
             if (enabledValue == 1)
             {
                 WifiManager::turnOn();
                 WifiManager::connect();
             }
             else
             {
                 WifiManager::disconnect();
                 WifiManager::turnOff();
             }
             ConfigManager::setConfigInt("Network", "Enabled", enabledValue);
         },
         []() -> std::string
         {
             return ConfigManager::getConfigInt("Network", "Enabled") ? "On" : "Off";
         },
         SettingType::BOOL, nullptr},
        {"Wi-Fi SSID",
         []() -> std::string
         {
             return ConfigManager::getConfigString("Network", "SSID");
         },
         [](const std::string &value)
         {
             ConfigManager::setConfigString("Network", "SSID", value);
         },
         []() -> std::string
         {
             return ConfigManager::getConfigString("Network", "SSID");
         },
         SettingType::STRING, nullptr},

        {"Wi-Fi Password",
         []() -> std::string
         {
             return ConfigManager::getConfigString("Network", "Password");
         },
         [](const std::string &value)
         {
             ConfigManager::setConfigString("Network", "Password", value);
         },
         []() -> std::string
         {
             std::string password = ConfigManager::getConfigString("Network", "Password");
             return password.empty() ? "" : "******";
         },
         SettingType::STRING, nullptr},
        {"Wi-Fi Debug",
         []() -> std::string
         {
             return "";
         },
         [this](const std::string &value)
         {
             appManager.launchApp("WifiDebug");
         },
         []() -> std::string
         {
             return "";
         },
         SettingType::BUTTON, nullptr},
    };
}

Settings::~Settings()
{
}

void Settings::launch()
{
    screenObj = get_app_container(appManager);

    lv_obj_t *tv = lv_tileview_create(screenObj);
    lv_obj_set_scrollbar_mode(tv, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(tv, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_add_flag(tv, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_radius(tv, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_pad_all(tv, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(tv, 0, LV_PART_MAIN);

    // General settings tile (default)
    lv_obj_t *generalTile = lv_tileview_add_tile(tv, 0, 0, LV_DIR_RIGHT);
    lv_obj_set_style_bg_color(generalTile, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_pad_all(generalTile, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(generalTile, 0, LV_PART_MAIN);
    create_settings_flex(generalTile, generalSettings, appManager);

    // Networks tile (right)
    lv_obj_t *networksTile = lv_tileview_add_tile(tv, 1, 0, LV_DIR_LEFT);
    lv_obj_set_style_bg_color(networksTile, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_pad_all(networksTile, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(networksTile, 0, LV_PART_MAIN);
    create_settings_flex(networksTile, networkSettings, appManager);
}

void Settings::close()
{
    if (screenObj && lv_obj_is_valid(screenObj))
    {
        lv_obj_del_async(screenObj);
        screenObj = NULL;
    }
}

void Settings::backgroundActivity()
{
    // currently nothing
}