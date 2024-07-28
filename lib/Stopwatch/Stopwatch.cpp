#include "Stopwatch.h"
#include "TextComponent.h"
#include <chrono>
#include <esp_log.h>
#include <memory>
#include "LvglMutex.h"
#include "app_screen.h"


Stopwatch::Stopwatch(AppManager& manager) 
    : IApp("StopWatch"), appManager(manager), isRunning(false), elapsed(0), 
      startTime(), timeListenerId(-1), screenObj(NULL), timeLabel(NULL), toggleButtonLabel(NULL) {}


Stopwatch::~Stopwatch() {
    
}

void Stopwatch::toggle_event_handler(lv_event_t * e)
{
    Stopwatch* stopwatch = (Stopwatch*)lv_event_get_user_data(e);
    if (stopwatch->isRunning) {
        stopwatch->stop();
    } else {
        stopwatch->start();
    }
}

void Stopwatch::launch() {

    screenObj = get_app_container(appManager);

    // Create LVGL labels
    lv_obj_t* clockTitleLabel = lv_label_create(screenObj);
    lv_label_set_text(clockTitleLabel, "StopWatch");
    lv_obj_align(clockTitleLabel, LV_ALIGN_TOP_MID, 0, 10); // Align title at the top middle

    // Set the style for the time label
    static lv_style_t style_title;
    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, lv_color_hex(0xFF0000)); // Red color
    lv_obj_add_style(clockTitleLabel, &style_title, 0);


    timeLabel = lv_label_create(screenObj);
    lv_label_set_text(timeLabel, "00:00:00.000");
    lv_obj_align(timeLabel, LV_ALIGN_CENTER, 0, 0); // Align time in the center

    // Set the style for the time
    static lv_style_t style_time;
    lv_style_init(&style_time);
    lv_style_set_text_color(&style_time, lv_color_hex(0xFFFFFF)); // White color
    lv_style_set_text_font(&style_time, &lv_font_montserrat_30); // Font size (twice the default size)
    lv_obj_add_style(timeLabel, &style_time, 0);

    // button
    lv_obj_t* toggleButton = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(toggleButton, toggle_event_handler, LV_EVENT_VALUE_CHANGED, this);
    lv_obj_align(toggleButton, LV_ALIGN_CENTER, 0, 70);
    lv_obj_add_flag(toggleButton, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_height(toggleButton, LV_SIZE_CONTENT);

    toggleButtonLabel = lv_label_create(toggleButton);
    lv_label_set_text(toggleButtonLabel, "Start");
    lv_obj_center(toggleButtonLabel);

    // add style
    static lv_style_t button_style;
    lv_style_init(&button_style);
    lv_style_set_text_font(&button_style, &lv_font_montserrat_16);
    lv_obj_add_style(toggleButtonLabel, &button_style, 0);

    updateDisplay();

    TimeManager& timeManager = appManager.getTimeManager();
    timeListenerId = timeManager.addTimeUpdateListener([this](const struct tm& timeinfo) {
        if (this->isRunning) {
            this->elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - this->startTime).count();
            LvglMutex::lock();
            this->updateDisplay();
            LvglMutex::unlock();
        }
    });
}

void Stopwatch::close() {
    // clean up
    if (timeListenerId != -1) {
        TimeManager& timeManager = appManager.getTimeManager();
        timeManager.removeTimeUpdateListener(timeListenerId);
        timeListenerId = -1;
    }

    // TODO: delete Display

}

void Stopwatch::start() {
    isRunning = true;
    startTime = std::chrono::system_clock::now();
    elapsed = 0;
    updateDisplay();
}

void Stopwatch::stop() {
    isRunning = false;
    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime).count();
    updateDisplay();
}

void Stopwatch::updateDisplay() {
    if (isRunning) {
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime).count();
    }
    
    int hours = (elapsed / 3600000);
    int minutes = (elapsed / 60000) % 60;
    int seconds = (elapsed / 1000) % 60;
    int milliseconds = elapsed % 1000;

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
    if (timeLabel && lv_obj_is_valid(timeLabel)) {
        lv_label_set_text(timeLabel, buffer);
    }

    if (toggleButtonLabel && lv_obj_is_valid(toggleButtonLabel)) {
       lv_label_set_text(toggleButtonLabel, + (isRunning ? "Stop" : "Start"));
    }

}


void Stopwatch::backgroundActivity() {
    // not used
}