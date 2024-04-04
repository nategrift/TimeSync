#include "Stopwatch.h"
#include "TextComponent.h"
#include <chrono>
#include <esp_log.h>


Stopwatch::Stopwatch(AppManager& manager) 
    : IApp(manager), appManager(manager), isRunning(false), elapsed(0), 
      startTime(), stopwatchDisplayId(-1), timeListenerId(-1) {}


Stopwatch::~Stopwatch() {
    if (timeListenerId != -1) {
        TimeManager& timeManager = appManager.getTimeManager();
        timeManager.removeTimeUpdateListener(timeListenerId);
    }
}

void Stopwatch::launch() {
    UIManager& uiManager = appManager.getUIManager();
    stopwatchDisplayId = uiManager.addOrUpdateComponent(TextComponent("--:--:--.---", true));

    updateDisplay();
    InputManager& inputManager = appManager.getInputManager();
    inputManager.addListener([this](InputEvent event) {
        if (event == InputEvent::BUTTON_CLICK) {
            if (this->isRunning) {
                this->stop();
            } else {
                this->start();
            }
            ESP_LOGI("Stopwatch", "Stopwatch %s", this->isRunning ? "started" : "stopped");
        }
    });

    TimeManager& timeManager = appManager.getTimeManager();
    timeListenerId = timeManager.addTimeUpdateListener([this](const struct tm& timeinfo) {
        if (this->isRunning) {
            this->elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - this->startTime).count();
            this->updateDisplay();
        }
    });
}

void Stopwatch::close() {
    UIManager& uiManager = appManager.getUIManager();
    uiManager.deleteComponent(stopwatchDisplayId);

    if (timeListenerId != -1) {
        TimeManager& timeManager = appManager.getTimeManager();
        timeManager.removeTimeUpdateListener(timeListenerId);
        timeListenerId = -1;
    }
}

void Stopwatch::start() {
    isRunning = true;
    startTime = std::chrono::system_clock::now();
    elapsed = 0;
    updateDisplay();
}

void Stopwatch::stop() {
    isRunning = false;
    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime).count();
    updateDisplay();
}

void Stopwatch::updateDisplay() {
    if (isRunning) {
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime).count();
    }
    
    int hours = (elapsed / 3600000);
    int minutes = (elapsed / 60000) % 60;
    int seconds = (elapsed / 1000) % 60;
    int milliseconds = elapsed % 1000;

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
    UIManager& uiManager = appManager.getUIManager();
    uiManager.updateComponentText(stopwatchDisplayId, std::string(buffer));
}


void Stopwatch::backgroundActivity() {
    // not used
}