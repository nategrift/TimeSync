#ifndef STOPWATCH_H
#define STOPWATCH_H

#include "IApp.h"
#include "AppManager.h"
#include <string>
#include <chrono>

class Stopwatch : public IApp {
private:
    AppManager& appManager;
    bool isRunning;
    long long elapsed;
    std::chrono::time_point<std::chrono::system_clock> startTime;
    int timeListenerId;

    lv_obj_t* screenObj;
    lv_obj_t* timeLabel;
    lv_obj_t* toggleButtonLabel;

public:
    Stopwatch(AppManager& manager);
    ~Stopwatch() override;

    void launch() override;
    void close() override;
    void backgroundActivity() override;

    static void toggle_event_handler(lv_event_t * e);

    void start();
    void stop();
    void updateDisplay();
};

#endif // STOPWATCH_H
