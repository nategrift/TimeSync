#include "TimeEventsManager.h"
#include <algorithm>
#include <sstream>
#include "esp_log.h"
#include "NotificationManager.h"

static const char* TAG = "TimeEventsManager";

std::vector<TimeEvent> TimeEventsManager::events;

static int8_t eventCounter = 0;

void TimeEventsManager::init() {
    deserializeTimeEvents();
}

int8_t TimeEventsManager::addTimeEvent(EventType type, time_t expireTime, const std::string& label, const std::string& appName) {
    TimeEvent newEvent;
    newEvent.id = generateUid();
    newEvent.type = type;
    newEvent.expireTime = expireTime;
    newEvent.label = label;
    newEvent.appName = appName;

    events.push_back(newEvent);
    sortEventsByTime();
    serializeTimeEvents();

    ESP_LOGI(TAG, "Added time event: ID=%d, Type=%d, Label='%s', AppName='%s'", 
             newEvent.id, static_cast<int>(type), label.c_str(), appName.c_str());

    return newEvent.id;
}

bool TimeEventsManager::deleteTimeEvent(int8_t id) {
    auto it = std::find_if(events.begin(), events.end(),
                           [&id](const TimeEvent& event) { return event.id == id; });
    if (it != events.end()) {
        events.erase(it);
        serializeTimeEvents();
        ESP_LOGI(TAG, "Deleted time event: ID=%d", id);
        return true;
    }
    ESP_LOGW(TAG, "Time event not found: ID=%d", id);
    return false;
}

void TimeEventsManager::clearAllEventsByType(EventType type) {
    events.erase(std::remove_if(events.begin(), events.end(),
                                [type](const TimeEvent& event) { return event.type == type; }),
                 events.end());
    serializeTimeEvents();
    ESP_LOGI(TAG, "Cleared all events of type: %d", static_cast<int>(type));
}

TimeEvent TimeEventsManager::getTimeEventById(int8_t id) {
    auto it = std::find_if(events.begin(), events.end(),
                           [&id](const TimeEvent& event) { return event.id == id; });
    if (it != events.end()) {
        return *it;
    }
    // Return an empty TimeEvent with id = -1 to indicate not found
    TimeEvent empty;
    empty.id = -1;
    return empty;
}

std::vector<TimeEvent> TimeEventsManager::getAllEventsByType(EventType type) {
    std::vector<TimeEvent> result;
    std::copy_if(events.begin(), events.end(), std::back_inserter(result),
                 [type](const TimeEvent& event) { return event.type == type; });
    return result;
}

std::vector<TimeEvent> TimeEventsManager::getExpiredTimeEvents() {
    std::vector<TimeEvent> expiredEvents;
    time_t now = time(nullptr);
    std::copy_if(events.begin(), events.end(), std::back_inserter(expiredEvents),
                 [now](const TimeEvent& event) { return event.expireTime <= now; });
    return expiredEvents;
}

void TimeEventsManager::serializeTimeEvents() {
    std::stringstream ss;
    for (const auto& event : events) {
        ss << static_cast<int>(event.id) << "," 
           << static_cast<int>(event.type) << "," 
           << event.expireTime << ","
           << event.label << "," 
           << event.appName << "\n";
    }
    bool success = file_manager_write_data("TimeEvents", "events_v2.csv", ss.str().c_str());
    if (!success) {
        ESP_LOGE(TAG, "Failed to serialize time events");
    }
}

void TimeEventsManager::deserializeTimeEvents() {
    char* data = file_manager_read_data("TimeEvents", "events_v2.csv");
    if (data == NULL) {
        ESP_LOGW(TAG, "No time events data found");
        return;
    }

    std::istringstream iss(data);
    std::string line;
    events.clear();

    while (std::getline(iss, line)) {
        // Validate the CSV line
        if (!isValidCsvLine(line)) {
            ESP_LOGW(TAG, "Invalid CSV line, skipping: %s", line.c_str());
            continue;
        }

        TimeEvent event;
        std::istringstream lineStream(line);
        std::string idStr, typeStr, expireTimeStr;

        if (std::getline(lineStream, idStr, ',') &&
            std::getline(lineStream, typeStr, ',') &&
            std::getline(lineStream, expireTimeStr, ',') &&
            std::getline(lineStream, event.label, ',') &&
            std::getline(lineStream, event.appName)) {

            event.id = static_cast<int8_t>(std::stoi(idStr));
            event.type = static_cast<EventType>(std::stoi(typeStr));
            event.expireTime = std::stoll(expireTimeStr);
            events.push_back(event);

            // Update eventCounter to ensure unique IDs
            if (event.id >= eventCounter) {
                eventCounter = event.id + 1;
            }
        } else {
            ESP_LOGE(TAG, "Error parsing line: %s", line.c_str()); 
        }
    }
    free(data);
    sortEventsByTime();
    ESP_LOGI(TAG, "Deserialized %zu time events", events.size());
}

bool TimeEventsManager::isValidCsvLine(const std::string& line) {
    std::istringstream lineStream(line);
    std::string field;
    int fieldCount = 0;

    while (std::getline(lineStream, field, ',') && fieldCount <= 5) {
        fieldCount++;
    }

    // A valid line should have 5 fields: id, type, expireTime, label, appName
    return fieldCount == 5;
}

int8_t TimeEventsManager::generateUid() {
    return eventCounter++;
}

void TimeEventsManager::sortEventsByTime() {
    std::sort(events.begin(), events.end(),
              [](const TimeEvent& a, const TimeEvent& b) { return a.expireTime < b.expireTime; });
}

void TimeEventsManager::checkExpiringEventsTask(void* pvParameters) {
    while (true) {
        checkAndNotifyExpiredEvents();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

bool TimeEventsManager::checkAndNotifyExpiredEvents() {
    std::vector<TimeEvent> expiredEvents = TimeEventsManager::getExpiredTimeEvents();
    ESP_LOGI(TAG, "Found %zu expired events", expiredEvents.size());
    bool notificationSent = false;

    for (const auto& event : expiredEvents) {
        // Check if notification is already sent
        if (NotificationManager::isNotificationSent(event.id)) {
            continue;
        }

        std::string title;
        if (event.type == EventType::TIMER) {
            title = "Timer";
        } else if (event.type == EventType::ALARM) {
            title = "Alarm";
        } else {
            title = "Event";
        }

        NotificationManager::createNotification(
            event.id,
            title, 
            event.label, 
            [event]() {
                TimeEventsManager::deleteTimeEvent(event.id);
            }, 
            false
        );
        notificationSent = true;
    }

    if (notificationSent) {
        serializeTimeEvents();
    }

    return notificationSent;
}
