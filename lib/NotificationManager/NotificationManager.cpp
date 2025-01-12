#include "NotificationManager.h"
#include <algorithm>
#include <sstream>
#include "esp_log.h"
#include "app_screen.h"
#include "lvgl.h"
#include "ui_components.h"
#include "VibrationDriver.h"
#include "buzzer_driver.h"

static const char* TAG = "NotificationManager";

std::vector<Notification> NotificationManager::notifications;

static int notificationCounter = 0;

int8_t NotificationManager::createNotification(int id, const std::string& title, const std::string& message, std::function<void()> onDismiss, bool important) {
    if (id == -1) {
        id = generateUid();
    }

    Notification newNotification;
    newNotification.id = id;
    newNotification.title = title;
    newNotification.message = message;
    newNotification.onDismiss = onDismiss;
    newNotification.important = important;

    ESP_LOGI(TAG, "Creating notification: ID=%d, Title='%s', Message='%s', Important=%s", 
             newNotification.id, title.c_str(), message.c_str(), important ? "true" : "false");

    if (notifications.empty()) {
        showNotification(newNotification);
    }

    notifications.push_back(newNotification);

    return newNotification.id;
}

void NotificationManager::dismissNotification(const int8_t id) {

    VibrationDriver::stop();
    stop_buzzer();

    auto it = std::find_if(notifications.begin(), notifications.end(),
                           [&id](const Notification& notification) { return notification.id == id; });
                           
    if (it != notifications.end()) {
        it->onDismiss();
        notifications.erase(it);
        ESP_LOGI(TAG, "Notification removed from queue. New queue size: %d", notifications.size());
        if (!notifications.empty()) {
            showNotification(notifications.front());
        }
    } else {
        ESP_LOGW(TAG, "Notification with ID %d not found", id);
    }
}

void NotificationManager::showNotification(Notification& notification) {
    VibrationDriver::incrementalVibration(30000);
    incremental_buzz_pattern(30000);

    lv_obj_t* screenObj = get_blank_screen(lv_scr_act());
    lv_obj_add_flag(screenObj, LV_OBJ_FLAG_FLOATING);

    lv_obj_t* titleLabel = lv_label_create(screenObj);
    lv_obj_align(titleLabel, LV_ALIGN_CENTER, 0, -70);
    lv_label_set_text(titleLabel, notification.title.c_str());

    static lv_style_t style_title;
    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, lv_color_hex(0xFF0000)); // Red color
    lv_obj_add_style(titleLabel, &style_title, 0);

    
    lv_obj_t* messageLabel = lv_label_create(screenObj);
    lv_obj_align(messageLabel, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(messageLabel, notification.message.c_str());

    static lv_style_t style_time;
    lv_style_init(&style_time);
    lv_style_set_text_color(&style_time, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_time, &lv_font_montserrat_16); 
    lv_obj_add_style(messageLabel, &style_time, 0);

    lv_obj_t* dismissButton = get_button(screenObj, "Dismiss");
    lv_obj_align(dismissButton, LV_ALIGN_CENTER, 0, 70);
    lv_obj_add_flag(dismissButton, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_event_cb(dismissButton, [](lv_event_t* e) {
        lv_obj_t * button = (lv_obj_t *)lv_event_get_target(e);
        Notification* notification = &notifications.front();
        NotificationManager::dismissNotification(notification->id);

        lv_obj_t* notification_container = lv_obj_get_parent(button);
        if (notification_container != nullptr && lv_obj_is_valid(notification_container)) {
            lv_obj_del_async(notification_container);
        }
    }, LV_EVENT_CLICKED, NULL);

    ESP_LOGI(TAG, "Displaying notification: %s", notification.title.c_str());
}

int8_t NotificationManager::generateUid() {
    return notificationCounter++;
}

bool NotificationManager::isNotificationSent(int id) {
    return std::any_of(notifications.begin(), notifications.end(),
        [&id](const Notification& notification) { return notification.id == id; });
}