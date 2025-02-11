#include "lvgl.h"
#include "Clock.h"
#include "AppManager.h"
#include "BatteryManager.h"

#include "esp_log.h"
#include "app_screen.h"
#include "ui_components.h"
#include "WifiManager.h"
#include "FitnessManager.h"

static const char* TAG = "Clock";

Clock::Clock(AppManager& manager)
    : IApp("Clock"),
      timeListenerId(-1),
      clockTimeLabel(NULL),
      screenObj(NULL),
      batteryLabel(NULL),
      batteryIcon(NULL),
      wifiIcon(NULL),
      stepsLabel(NULL),
      appManager(manager),
      batteryManager(manager.getBatteryManager()),
      batteryUpdateTimer(NULL) {}

Clock::~Clock() {

}

void Clock::launch() {
    screenObj = get_app_container(appManager);

    // Create LVGL labels
    lv_obj_t* clockTitleLabel = lv_label_create(screenObj);
    lv_label_set_text(clockTitleLabel, "Clock");
    lv_obj_align(clockTitleLabel, LV_ALIGN_TOP_MID, 0, 10); // Align title at the top middle

    // Set the style for the clock title
    static lv_style_t style_title;
    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, lv_color_hex(0xFF0000)); // Red color
    lv_obj_add_style(clockTitleLabel, &style_title, 0);
    
    stepsLabel = lv_label_create(screenObj);
    lv_label_set_text(stepsLabel, "0 Steps");
    lv_obj_align(stepsLabel, LV_ALIGN_TOP_MID, 0, 30); // Align title at the top middle

    // Set the style for the clock title
    static lv_style_t steps_style;
    lv_style_init(&steps_style);
    lv_style_set_text_color(&steps_style, COLOR_MUTED_TEXT); // Red color
    lv_obj_add_style(stepsLabel, &steps_style, 0);

    clockTimeLabel = lv_label_create(screenObj);
    lv_label_set_text(clockTimeLabel, "--:--:-- --");
    lv_obj_align(clockTimeLabel, LV_ALIGN_CENTER, 0, 0); // Align time in the center

    // Set the style for the clock time
    static lv_style_t style_time;
    lv_style_init(&style_time);
    lv_style_set_text_color(&style_time, lv_color_hex(0xFFFFFF)); // White color
    lv_style_set_text_font(&style_time, &lv_font_montserrat_30); // Font size (twice the default size)

    lv_obj_add_style(clockTimeLabel, &style_time, 0);

    clockDateLabel = lv_label_create(screenObj);
    lv_label_set_text(clockDateLabel, "--- --");
    lv_obj_align(clockDateLabel, LV_ALIGN_CENTER, 0, 25);

    static lv_style_t style_date;
    lv_style_init(&style_date);
    lv_style_set_text_color(&style_date, COLOR_MUTED_TEXT);
    lv_style_set_text_font(&style_date, &lv_font_montserrat_12);

    lv_obj_add_style(clockDateLabel, &style_date, 0);


    static lv_style_t style_battery;
    lv_style_init(&style_battery);
    lv_style_set_text_color(&style_battery, lv_color_hex(0xFFFFFF));

    batteryLabel = lv_label_create(screenObj);
    lv_label_set_text(batteryLabel, "--%"); // Initial battery percentage
    lv_obj_add_style(batteryLabel, &style_battery, 0);
    lv_obj_align(batteryLabel, LV_ALIGN_BOTTOM_MID, 25, -15);

    batteryIcon =  lv_label_create(screenObj);
    lv_obj_add_style(batteryIcon, &style_battery, 0);
    lv_obj_align(batteryIcon, LV_ALIGN_BOTTOM_MID, -5, -15);

    wifiIcon = lv_label_create(screenObj);
    lv_obj_add_style(wifiIcon, &style_battery, 0);
    lv_obj_align(wifiIcon, LV_ALIGN_BOTTOM_MID, -30, -15);
    lv_label_set_text(wifiIcon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifiIcon, COLOR_TEXT, LV_PART_MAIN);

    // // Set up the time update listener
    // timeListenerId = TimeManager::addTimeUpdateListener([this](const struct tm& timeinfo) {
    //     this->handleTimeUpdate(timeinfo);
    // });

    // set default time
    // unsafe calling because we already in lvgl loop
    this->handleTimeUpdate(TimeManager::getLocalTimeInfo());
    

    // Update the battery level immediately and periodically
    updateBatteryLevel();
    updateWifiIcon();
    updateSteps();
    // batteryUpdateTimer = lv_timer_create([](lv_timer_t* timer) {
    //     Clock* clock = static_cast<Clock*>(timer->user_data);
    //     clock->updateBatteryLevel();
    //     clock->updateWifiIcon();
    //     clock->updateSteps();
    // }, 4000, this);
}

void Clock::close() {
    // Remove the time update listener
    if (timeListenerId != -1) {
        TimeManager::removeTimeUpdateListener(timeListenerId);
        timeListenerId = -1;
    }

    // Stop and delete the battery update timer
    if (batteryUpdateTimer) {
        lv_timer_pause(batteryUpdateTimer);
        lv_timer_del(batteryUpdateTimer);
        batteryUpdateTimer = NULL;
    }

    // Delete the screen object and reset pointers
    if (screenObj && lv_obj_is_valid(screenObj)) {
        lv_obj_del_async(screenObj);
        screenObj = NULL;
        batteryIcon = NULL;
        batteryLabel = NULL;
        clockDateLabel = NULL;
        clockTimeLabel = NULL;
    }
}

void Clock::handleTimeUpdate(const struct tm& timeinfo) {
    // Format time as "HH:MM:SS AM/PM"
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%I:%M:%S %p", &timeinfo);

    // Format date as "Mon dd"
    char date_buf[16];
    strftime(date_buf, sizeof(date_buf), "%b %d %Y", &timeinfo);  // %b is abbreviated month name

    // Update the time label
    if (clockTimeLabel && lv_obj_is_valid(clockTimeLabel)) {
        lv_label_set_text(clockTimeLabel, time_buf);
        lv_obj_align(clockTimeLabel, LV_ALIGN_CENTER, 0, 0);\
    }

    // Update the date label
    if (clockDateLabel && lv_obj_is_valid(clockDateLabel)) {
        lv_label_set_text(clockDateLabel, date_buf);
        lv_obj_align(clockDateLabel, LV_ALIGN_CENTER, 0, 25);
    }
}

void Clock::updateBatteryLevel() {
    uint8_t battery_level = batteryManager.getBatteryLevel();
    bool battery_charging = batteryManager.getBatteryCharging();
    char battery_text[10] = "PWR";
    if (!battery_charging) {
        snprintf(battery_text, sizeof(battery_text), "%d%%", battery_level);
    }

    if (batteryLabel && lv_obj_is_valid(batteryLabel)) {
        lv_label_set_text(batteryLabel, battery_text);
        // lv_obj_align(batteryLabel, LV_ALIGN_BOTTOM_MID, 10, -10);
    }

    if (batteryIcon) {
        const char* icon = LV_SYMBOL_BATTERY_EMPTY;
        if (battery_level > 80) {
            icon = LV_SYMBOL_BATTERY_FULL;
        } else if (battery_level > 60) {
            icon = LV_SYMBOL_BATTERY_3;
        } else if (battery_level > 40) {
            icon = LV_SYMBOL_BATTERY_2;
        } else if (battery_level > 20) {
            icon = LV_SYMBOL_BATTERY_1;
        }

        if (battery_charging) {
            icon = LV_SYMBOL_USB;
        }
        lv_label_set_text(batteryIcon, icon);
        // lv_obj_align(batteryIcon, LV_ALIGN_BOTTOM_MID, -20, -10);
    }
}

void Clock::updateWifiIcon() {
    if (wifiIcon && lv_obj_is_valid(wifiIcon)) {
        bool isConnected = WifiManager::isConnected();
        bool isOn = WifiManager::isOn();
        lv_color_t color = COLOR_TEXT;
        if (!isOn) {
            color = COLOR_MUTED_TEXT;
        } else if (isOn && !isConnected) {
            color = COLOR_ERROR;
        } else {
            int status = WifiManager::getSignalStrength();
            if (status > -75) {
                color = COLOR_SUCCESS;
            } else {
                color = COLOR_WARNING;
            }
        }

        lv_obj_set_style_text_color(wifiIcon, color, LV_PART_MAIN);
    }
}

void Clock::updateSteps() {
    int steps = ConfigManager::getConfigInt(FitnessManager::KEY, FitnessManager::DAILY_STEPS_KEY);
    std::string stepText = std::to_string(steps) + " Steps";
    lv_label_set_text(stepsLabel, stepText.c_str());
}

void Clock::backgroundActivity() {
    // currently nothing
}
