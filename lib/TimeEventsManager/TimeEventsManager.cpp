#include "TimeEventsManager.h"
#include <algorithm>
#include <sstream>
#include "esp_log.h"
#include "NotificationManager.h"

static const char* TAG = "TimeEventsManager";

std::vector<TimeEvent> TimeEventsManager::events;
FileManager TimeEventsManager::fileManager;

static int8_t eventCounter = 0; // Add this line

void TimeEventsManager::init() {
    deserializeTimeEvents();
}

int8_t TimeEventsManager::addTimeEvent(EventType type, time_t endTime, const std::string& label) {
    TimeEvent newEvent;
    newEvent.id = generateUid(); // Modify this line
    newEvent.type = type;
    newEvent.startTime = time(nullptr);
    newEvent.endTime = endTime;
    newEvent.label = label;
    newEvent.active = true;

    events.push_back(newEvent);
    sortEventsByTime();
    serializeTimeEvents();

    return newEvent.id;
}

bool TimeEventsManager::cancelEvent(int8_t id) {
    auto it = std::find_if(events.begin(), events.end(),
                           [&id](const TimeEvent& event) { return event.id == id; });
    if (it != events.end()) {
        events.erase(it);
        serializeTimeEvents();
        return true;
    }
    return false;
}

bool TimeEventsManager::editTimeEvent(int8_t id, time_t endTime, const std::string& label) {
    auto it = std::find_if(events.begin(), events.end(),
                           [&id](const TimeEvent& event) { return event.id == id; });
    if (it != events.end()) {
        it->endTime = endTime;
        it->label = label;
        sortEventsByTime();
        serializeTimeEvents();
        return true;
    }
    return false;
}

bool TimeEventsManager::toggleTimeEvent(int8_t id) {
    auto it = std::find_if(events.begin(), events.end(),
                           [&id](const TimeEvent& event) { return event.id == id; });
    if (it != events.end()) {
        it->active = !it->active;
        serializeTimeEvents();
        return true;
    }
    return false;
}

std::vector<TimeEvent> TimeEventsManager::getAllEventsByType(EventType type) {
    std::vector<TimeEvent> result;
    std::copy_if(events.begin(), events.end(), std::back_inserter(result),
                 [type](const TimeEvent& event) { return event.type == type; });
    return result;
}

std::vector<TimeEvent> TimeEventsManager::getAllActiveEventsByType(EventType type) {
    std::vector<TimeEvent> result;
    std::copy_if(events.begin(), events.end(), std::back_inserter(result),
                 [type](const TimeEvent& event) { return event.type == type && event.active; });
    return result;
}

void TimeEventsManager::clearAllEventsByType(EventType type) {
    events.erase(std::remove_if(events.begin(), events.end(),
                                [type](const TimeEvent& event) { return event.type == type; }),
                 events.end());
    serializeTimeEvents();
}

TimeEvent TimeEventsManager::getTimeEventById(int8_t id) {
    auto it = std::find_if(events.begin(), events.end(),
                           [&id](const TimeEvent& event) { return event.id == id; });
    if (it != events.end()) {
        return *it;
    }
    return TimeEvent(); // Return an empty TimeEvent if not found
}

std::vector<TimeEvent> TimeEventsManager::getExpiredTimeEvents() {
    std::vector<TimeEvent> expiredEvents;
    time_t now = time(nullptr);
    std::copy_if(events.begin(), events.end(), std::back_inserter(expiredEvents),
                 [now](const TimeEvent& event) { return event.active && event.endTime <= now; });
    return expiredEvents;
}

void TimeEventsManager::serializeTimeEvents() {
    std::stringstream ss;
    for (const auto& event : events) {
        ss << event.id << "," << static_cast<int>(event.type) << "," << event.startTime << ","
           << event.endTime << "," << event.label << "," << (event.active ? "1" : "0") << "\n";
    }
    bool success = fileManager.writeData("TimeEvents", "events.csv", ss.str());
    if (!success) {
        ESP_LOGE(TAG, "Failed to serialize time events");
    }
}

void TimeEventsManager::deserializeTimeEvents() {
    std::string data = fileManager.readData("TimeEvents", "events.csv");
    if (data.empty()) {
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
        std::string idStr, typeStr, startTimeStr, endTimeStr, activeStr;

        if (std::getline(lineStream, idStr, ',') &&
            std::getline(lineStream, typeStr, ',') &&
            std::getline(lineStream, startTimeStr, ',') &&
            std::getline(lineStream, endTimeStr, ',') &&
            std::getline(lineStream, event.label, ',') &&
            std::getline(lineStream, activeStr)) {

            event.id = static_cast<int8_t>(std::stoi(idStr)); // Modify this line
            event.type = static_cast<EventType>(std::stoi(typeStr));
            event.startTime = std::stoll(startTimeStr);
            event.endTime = std::stoll(endTimeStr);
            event.active = (activeStr == "1");
            events.push_back(event);

            // Update eventCounter to ensure unique IDs
            if (event.id >= eventCounter) {
                eventCounter = event.id + 1;
            }
        } else {
            ESP_LOGE(TAG, "Error parsing line: %s", line.c_str()); 
        }
    }

    sortEventsByTime();
    ESP_LOGI(TAG, "Deserialized %zu time events", events.size());
}

// Add this new method to the TimeEventsManager class
bool TimeEventsManager::isValidCsvLine(const std::string& line) {
    std::istringstream lineStream(line);
    std::string field;
    int fieldCount = 0;

    while (std::getline(lineStream, field, ',') && fieldCount <= 6) {
        fieldCount++;
    }

    // A valid line should have 6 fields: id, type, startTime, endTime, label, active
    return fieldCount == 6;
}

int8_t TimeEventsManager::generateUid() {
    return eventCounter++; // Modify this line
}

void TimeEventsManager::sortEventsByTime() {
    std::sort(events.begin(), events.end(),
              [](const TimeEvent& a, const TimeEvent& b) { return a.endTime < b.endTime; });
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

        // check if notification is already sent
        if (NotificationManager::isNotificationSent(event.id)) {
            continue;
        }

        NotificationManager::createNotification(
            event.id,
            "Event Expired", 
            event.label, 
            [event]() {
                TimeEventsManager::cancelEvent(event.id);
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