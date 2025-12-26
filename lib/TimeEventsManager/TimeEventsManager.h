#ifndef TIMEEVENTSMANAGER_H
#define TIMEEVENTSMANAGER_H

#include <vector>
#include <string>
#include <ctime>
#include "TimeManager.h"
#include "FileManager.h"

enum class EventType {
    ALARM,
    TIMER
};

struct TimeEvent {
    int8_t id;
    EventType type;
    time_t expireTime;
    std::string label;
    std::string appName;
};

class TimeEventsManager {
public:
    static void init();
    
    // Add a new time event, returns the generated ID
    static int8_t addTimeEvent(EventType type, time_t expireTime, const std::string& label, const std::string& appName);
    
    // Delete a time event by ID
    static bool deleteTimeEvent(int8_t id);
    
    // Clear all events of a specific type
    static void clearAllEventsByType(EventType type);
    
    // Get a single time event by ID
    static TimeEvent getTimeEventById(int8_t id);
    
    // Get all events of a specific type
    static std::vector<TimeEvent> getAllEventsByType(EventType type);
    
    // Get all expired events
    static std::vector<TimeEvent> getExpiredTimeEvents();
    
    // Task for checking expiring events
    static void checkExpiringEventsTask(void* pvParameters);
    
    // Check and notify for expired events
    static bool checkAndNotifyExpiredEvents();

private:
    static std::vector<TimeEvent> events;

    static void serializeTimeEvents();
    static void deserializeTimeEvents();
    static int8_t generateUid();
    static void sortEventsByTime();
    static bool isValidCsvLine(const std::string& line);
};

#endif // TIMEEVENTSMANAGER_H
