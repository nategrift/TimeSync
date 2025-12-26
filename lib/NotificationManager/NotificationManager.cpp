#include "NotificationManager.h"
#include <algorithm>
#include "esp_log.h"
#include "VibrationDriver.h"
#include "buzzer_driver.h"
#include "app_runtime.h"

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
        if (it->onDismiss) {
            it->onDismiss();
        }
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
    // Queue notification to be shown via Lua UI
    // Vibration and sound will be started by the Lua task after UI is shown
    bool queued = showLuaNotification(
        notification.id,
        notification.title.c_str(),
        notification.message.c_str(),
        notification.important
    );

    if (queued) {
        ESP_LOGI(TAG, "Queued notification for display: %s", notification.title.c_str());
    } else {
        ESP_LOGW(TAG, "Could not queue notification (Lua not ready): %s", notification.title.c_str());
        // Still start vibration/sound even if Lua isn't ready
        VibrationDriver::incrementalVibration(30000);
        incremental_buzz_pattern(30000);
    }
}

int8_t NotificationManager::generateUid() {
    return notificationCounter++;
}

bool NotificationManager::isNotificationSent(int id) {
    return std::any_of(notifications.begin(), notifications.end(),
        [&id](const Notification& notification) { return notification.id == id; });
}