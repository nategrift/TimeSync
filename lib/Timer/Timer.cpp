#include "Timer.h"
#include "AppManager.h"
#include "esp_log.h"
#include "app_screen.h"
#include "Settings.h"


static const char* TAG = "Timer";

int default_seconds = 5;


Timer::Timer(AppManager& manager)
    : IApp("Timer"),
      screenObj(NULL),
      timerLabel(NULL),
      stateButton(NULL),
      appManager(manager),
      timeListenerId(-1),
      remainingSeconds(default_seconds),
      previousScreen(NULL) {}

Timer::~Timer() {}

void Timer::launch() {
    screenObj = get_app_container(appManager);

    // Create LVGL labels and buttons
    lv_obj_t* titleLabel = lv_label_create(screenObj);
    lv_label_set_text(titleLabel, "Timer");
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 10);

    // Set the style for the clock title
    static lv_style_t style_title;
    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, lv_color_hex(0xFF0000)); // Red color
    lv_obj_add_style(titleLabel, &style_title, 0);

    timerLabel = lv_label_create(screenObj);
    lv_label_set_text(timerLabel, "00:00:00");
    lv_obj_center(timerLabel);

    // Set the style for the time
    static lv_style_t style_time;
    lv_style_init(&style_time);
    lv_style_set_text_color(&style_time, lv_color_hex(0xFFFFFF)); // White color
    lv_style_set_text_font(&style_time, &lv_font_montserrat_30); // Font size (twice the default size)
    lv_obj_add_style(timerLabel, &style_time, 0);

    // Add click event to timerLabel
    lv_obj_add_event_cb(timerLabel, timerLabelClickHandler, LV_EVENT_CLICKED, this);

    stateButton = get_button(screenObj, "Start");
    lv_obj_align(stateButton, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(stateButton, stateButtonHandler, LV_EVENT_CLICKED, this);

    // Set up the time update listener
    timeListenerId = TimeManager::addTimeUpdateListener([this](const struct tm& timeinfo) {
        this->handleTimeUpdate(timeinfo);
    });

    updateTimerDisplay();
}

bool Timer::getTimer(TimeEvent& timeEvent) {
    std::vector<TimeEvent> activeEvents = TimeEventsManager::getAllActiveEventsByType(EventType::TIMER);
    if (activeEvents.empty()) {
        return false;
    } else {
        timeEvent = activeEvents[0];
        return true;
    }
}

bool Timer::isTimerRunning() {
    TimeEvent timeEvent;
    return getTimer(timeEvent);
}

void Timer::close() {
    if (timeListenerId != -1) {
        TimeManager::removeTimeUpdateListener(timeListenerId);
        timeListenerId = -1;
    }

    if (screenObj && lv_obj_is_valid(screenObj)) {
        lv_obj_del_async(screenObj);
        screenObj = NULL;
        timerLabel = NULL;
        stateButton = NULL;
    }
}

void Timer::backgroundActivity() {
    // Currently nothing
}

void Timer::updateTimerDisplay() {
    if (timerLabel && lv_obj_is_valid(timerLabel)) {
        int hours = remainingSeconds / 3600;
        int minutes = (remainingSeconds % 3600) / 60;
        int seconds = remainingSeconds % 60;

        char time_buf[16];
        snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", hours, minutes, seconds);
        lv_label_set_text(timerLabel, time_buf);
        lv_obj_align(timerLabel, LV_ALIGN_CENTER, 0, 0);
    }

     if (stateButton) {
        lv_obj_t* label = lv_obj_get_child(stateButton, 0);
        if (label) {
            lv_label_set_text(label, isTimerRunning() ? "Stop" : "Start");
        }
    }
}

void Timer::startTimer() {
    char event_description[32];
    snprintf(event_description, sizeof(event_description), "Timer %d s", default_seconds);

    TimeEventsManager::addTimeEvent(EventType::TIMER, time(nullptr) + default_seconds, event_description);

    updateTimerDisplay();
}

void Timer::stopTimer() {
    TimeEvent timeEvent;
    if (getTimer(timeEvent)) {
        TimeEventsManager::cancelEvent(timeEvent.id);
    }
    updateTimerDisplay();
}

void Timer::setTimerDuration(int seconds) {
    remainingSeconds = seconds;
    updateTimerDisplay();
}

void Timer::timerLabelClickHandler(lv_event_t* e) {
    Timer* timerApp = static_cast<Timer*>(lv_event_get_user_data(e));
    if (!timerApp->isTimerRunning()) {
        // Call the function to set the timer time (to be implemented later)
        timerApp->showSetTimerDialog();
    }
}

void Timer::stateButtonHandler(lv_event_t* e) {
    Timer* timerApp = static_cast<Timer*>(lv_event_get_user_data(e));
    if (timerApp->isTimerRunning()) {
        timerApp->stopTimer();
    } else {
        timerApp->startTimer();
    }
}

void Timer::showSetTimerDialog() {
    // This function will be implemented later to show a dialog for setting the timer duration
    ESP_LOGI(TAG, "showSetTimerDialog called");


    Setting timeSetting;
    timeSetting.title = "Set Timer Duration";
    timeSetting.type = SettingType::TIME;
    timeSetting.readCallback = [this]() {
        int hours = default_seconds / 3600;
        int minutes = (default_seconds % 3600) / 60;
        int seconds = default_seconds % 60;
        char time_buf[16];
        snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", hours, minutes, seconds);
        return std::string(time_buf);
    };
    timeSetting.writeCallback = [this](const std::string& newTime) {
        int hours, minutes, seconds;
        sscanf(newTime.c_str(), "%d:%d:%d", &hours, &minutes, &seconds);
        default_seconds = hours * 3600 + minutes * 60 + seconds;
        updateTimerDisplay();

        if (previousScreen) {
            lv_scr_load_anim(previousScreen, LV_SCR_LOAD_ANIM_OVER_RIGHT, 500, 0, true);
            previousScreen = nullptr;
        }
    };

    lv_obj_t* timeEditScreen = create_time_edit_screen(&timeSetting);
    previousScreen = lv_scr_act();
    lv_scr_load_anim(timeEditScreen, LV_SCR_LOAD_ANIM_OVER_LEFT, 500, 0, false);
}

void Timer::handleTimeUpdate(const struct tm& timeinfo) {
    TimeEvent timeEvent;
    bool timerRunning = getTimer(timeEvent);
    time_t currentTime = mktime(const_cast<struct tm*>(&timeinfo));

    if (timerRunning && timeEvent.endTime > currentTime) {
        time_t eventEndTime = timeEvent.endTime;
        remainingSeconds = difftime(eventEndTime, currentTime);
    } else {
        remainingSeconds = default_seconds;
    }

    updateTimerDisplay();
}