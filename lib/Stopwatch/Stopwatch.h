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
    int stopwatchTimeId;
    int stopwatchTitleId;
    int timeListenerId;
    int inputListenerId;

public:
    Stopwatch(AppManager& manager);
    ~Stopwatch() override;

    void launch() override;
    void close() override;
    void backgroundActivity() override;

    void start();
    void stop();
    void updateDisplay();
};

#endif // STOPWATCH_H
