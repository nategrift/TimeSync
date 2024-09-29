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
    time_t startTime;
    time_t endTime;
    std::string label;
    bool active;
};

class TimeEventsManager {
public:
    static void init();
    static int8_t addTimeEvent(EventType type, time_t endTime, const std::string& label);
    static bool cancelEvent(int8_t id);
    static bool editTimeEvent(int8_t id, time_t endTime, const std::string& label);
    static bool toggleTimeEvent(int8_t id);
    static std::vector<TimeEvent> getAllEventsByType(EventType type);
    static std::vector<TimeEvent> getAllActiveEventsByType(EventType type);
    static void clearAllEventsByType(EventType type);
    static TimeEvent getTimeEventById(int8_t id);
    static std::vector<TimeEvent> getExpiredTimeEvents();
    static void checkExpiringEventsTask(void* pvParameters);
    static bool checkAndNotifyExpiredEvents();

private:
    static std::vector<TimeEvent> events;
    static FileManager fileManager;

    static void serializeTimeEvents();
    static void deserializeTimeEvents();
    static std::string generateUid(time_t endTime, const std::string& label);
    static void sortEventsByTime();
    static int8_t generateUid();
    static bool isValidCsvLine(const std::string& line);
};

#endif // TIMEEVENTSMANAGER_H