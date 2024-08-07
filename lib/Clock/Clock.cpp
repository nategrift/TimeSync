#include "Clock.h"
#include "AppManager.h"
#include "TimeManager.h"
#include "BatteryManager.h"
#include "LvglMutex.h"

#include "esp_log.h"
#include "app_screen.h"

static const char* TAG = "Clock";

Clock::Clock(AppManager& manager)
    : IApp("Clock"), // Initialize the app name
      timeListenerId(-1),
      clockTimeLabel(NULL),
      screenObj(NULL),
      batteryLabel(NULL),
      batteryIcon(NULL),
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

    clockTimeLabel = lv_label_create(screenObj);
    lv_label_set_text(clockTimeLabel, "--:--:-- --");
    lv_obj_align(clockTimeLabel, LV_ALIGN_CENTER, 0, 0); // Align time in the center

    // Set the style for the clock time
    static lv_style_t style_time;
    lv_style_init(&style_time);
    lv_style_set_text_color(&style_time, lv_color_hex(0xFFFFFF)); // White color
    lv_style_set_text_font(&style_time, &lv_font_montserrat_30); // Font size (twice the default size)

    lv_obj_add_style(clockTimeLabel, &style_time, 0);

    static lv_style_t style_battery;
    lv_style_init(&style_battery);
    lv_style_set_text_color(&style_battery, lv_color_hex(0xFFFFFF));

    batteryLabel = lv_label_create(screenObj);
    lv_label_set_text(batteryLabel, "--%"); // Initial battery percentage
    lv_obj_add_style(batteryLabel, &style_battery, 0);
    lv_obj_align(batteryLabel, LV_ALIGN_BOTTOM_MID, 0, -10);

    // Set up the time update listener
    TimeManager& timeManager = appManager.getTimeManager();
    timeListenerId = timeManager.addTimeUpdateListener([this](const struct tm& timeinfo) {
        this->handleTimeUpdate(timeinfo);
    });

    // Update the battery level immediately and periodically
    updateBatteryLevel();
    batteryUpdateTimer = lv_timer_create([](lv_timer_t* timer) {
        Clock* clock = static_cast<Clock*>(timer->user_data);
        clock->updateBatteryLevel();
    }, 15000, this);
}

void Clock::close() {
    if (timeListenerId != -1) {
        TimeManager& timeManager = appManager.getTimeManager();
        timeManager.removeTimeUpdateListener(timeListenerId);
        timeListenerId = -1;
    }
    if (batteryUpdateTimer) {
        lv_timer_del(batteryUpdateTimer);
        batteryUpdateTimer = NULL;
    }

    if (clockTimeLabel) {
        lv_obj_del(clockTimeLabel);
        clockTimeLabel = NULL;
    }
    if (batteryLabel) {
        lv_obj_del(batteryLabel);
        batteryLabel = NULL;
    }
    if (batteryIcon) {
        lv_obj_del(batteryIcon);
        batteryIcon = NULL;
    }
    if (screenObj) {
        lv_obj_del(screenObj);
        screenObj = NULL;
    }
}

void Clock::handleTimeUpdate(const struct tm& timeinfo) {
    LvglMutex::lock();
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%I:%M:%S %p", &timeinfo);

    if (lv_obj_is_valid(clockTimeLabel)) {
        lv_label_set_text(clockTimeLabel, strftime_buf);
        lv_obj_align(clockTimeLabel, LV_ALIGN_CENTER, 0, 0); // Re-align to ensure it's centered
    }
    LvglMutex::unlock();
}

void Clock::updateBatteryLevel() {
    uint8_t battery_level = batteryManager.getBatteryLevel();
    char battery_text[10];
    snprintf(battery_text, sizeof(battery_text), "%d%%", battery_level);

    if (batteryLabel) {
        lv_label_set_text(batteryLabel, battery_text);
        lv_obj_align(batteryLabel, LV_ALIGN_BOTTOM_MID, 0, -10); // Re-align to ensure it's positioned correctly
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
        lv_label_set_text(batteryIcon, icon);
        lv_obj_align(batteryIcon, LV_ALIGN_BOTTOM_LEFT, 10, -10); // Re-align to ensure it's positioned correctly
    }
}

void Clock::backgroundActivity() {
    // currently nothing
}
