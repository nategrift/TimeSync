#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <vector>
#include <string>
#include <functional>

struct Notification {
    int8_t id;
    std::string title;
    std::string message;
    std::function<void()> onDismiss;
    bool important;
};


class NotificationManager {
public:
    static int8_t createNotification(int id, const std::string& title, const std::string& message, std::function<void()> onDismiss, bool important);
    static void dismissNotification(const int8_t id);
    static bool isNotificationSent(int id);

private:
    static std::vector<Notification> notifications;
    static void showNotification(Notification& notification);
    static int8_t generateUid(); 
};

#endif // NOTIFICATIONMANAGER_H