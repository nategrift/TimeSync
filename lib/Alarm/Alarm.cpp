#include "Alarm.h"
#include <sstream>

Alarm::Alarm(AppManager& manager) : IApp(manager), isLaunched(false), alarmCheckTaskHandle(nullptr), selectedAlarmId(-1) {
    deserializeAlarms();
}

Alarm::~Alarm() {
    if (alarmCheckTaskHandle != nullptr) {
        vTaskDelete(alarmCheckTaskHandle);
    }
}

void Alarm::launch() {
    isLaunched = true;
    transitionToState(AlarmState::DEFAULT);
}

void Alarm::close() {
    isLaunched = false;
    transitionToState(AlarmState::DEFAULT);
}

void Alarm::setAlarm(int hour, int minute) {
    // Prevent Duplicates
    for (auto& alarm : alarms) {
        if (alarm.hour == hour && alarm.minute == minute) {
            // if we have a duplicate, just enable it
            alarm.enabled = true;
            serializeAlarms();
            return;
        }
    }

    alarms.push_back({hour, minute, true});
    if (!alarmCheckTaskHandle) {
        xTaskCreate(alarmCheckTask, "AlarmCheckTask", 2048, this, 5, &alarmCheckTaskHandle);
    }
    serializeAlarms(); 
}

void Alarm::setAlarmEnabled(int hour, int minute, bool enabled) {
    for (auto& alarm : alarms) {
        if (alarm.hour == hour && alarm.minute == minute) {
            alarm.enabled = enabled;
            serializeAlarms(); 
            return;
        }
    }

    ESP_LOGE("Alarm", "Setting an alarm enabled state failed. Unable to find alarm.");
}

void Alarm::deleteAlarm(int hour, int minute) {
    alarms.erase(std::remove_if(alarms.begin(), alarms.end(), [hour, minute](const AlarmTime& alarm) {
        return alarm.hour == hour && alarm.minute == minute;
    }), alarms.end());
    if (alarms.empty() && alarmCheckTaskHandle != nullptr) {
        vTaskDelete(alarmCheckTaskHandle);
        alarmCheckTaskHandle = nullptr;
    }
    serializeAlarms(); 
}

bool Alarm::isAlarmOn() const {
    return !alarms.empty();
}

void Alarm::transitionToState(AlarmState newState) {
    UIManager& uiManager = appManager.getUIManager();
    for (auto& component : activeComponents) {
        uiManager.deleteComponent(component);
    }
    activeComponents.clear();
    currentState = newState;
    switch (newState) {
        case AlarmState::DEFAULT:
            showDefaultState();
            break;
        case AlarmState::NEW:
            enterNewAlarmState();
            break;
        case AlarmState::OPTIONS:
            enterOptionsState();
            break;
    }
}

void Alarm::showDefaultState() {
    auto alarmList = std::make_shared<ListComponent>(appManager);
    int id = 0;
    for (const auto& alarm : alarms) {
        std::string alarmTime = "[" + std::string(alarm.enabled ? "X" : " ") + "] " + (alarm.hour < 10 ? "0" : "") + std::to_string(alarm.hour) + ":" + (alarm.minute < 10 ? "0" : "") + std::to_string(alarm.minute);
        alarmList->addItem(id++, std::make_shared<TextComponent>(alarmTime));
    }
    // Add option to create a new alarm
    alarmList->addItem(id, std::make_shared<TextComponent>("< New >"));

    alarmList->setOnSelectCallback([this](int alarmId) {
        if (alarmId == alarms.size()) {
            transitionToState(AlarmState::NEW);
        } else {
            selectedAlarmId = alarmId;
            transitionToState(AlarmState::OPTIONS);
        }
    });

    
    UIManager& uiManager = appManager.getUIManager();
    int alarmListId = uiManager.addOrUpdateComponent(alarmList);
    activeComponents.push_back(alarmListId);
}

void Alarm::enterNewAlarmState() {
    // Simplified pseudo-implementation. You'll need actual UI interaction handling here.
    // For demonstration, let's just add a dummy alarm at 7:30.
    setAlarm(alarms.size()+1, alarms.size()+4);
    transitionToState(AlarmState::DEFAULT);
}

void Alarm::enterOptionsState() {
    if (selectedAlarmId < 0 || selectedAlarmId >= alarms.size()) {
        ESP_LOGE("Alarm", "Invalid alarm selection.");
        return;
    }

    auto optionsList = std::make_shared<ListComponent>(appManager);
    optionsList->addItem(0, std::make_shared<TextComponent>("Enable"));
    optionsList->addItem(1, std::make_shared<TextComponent>("Disable"));
    optionsList->addItem(2, std::make_shared<TextComponent>("Delete"));

    optionsList->setOnSelectCallback([this](int optionId) {
        switch (optionId) {
            case 0:  // Enable
                setAlarmEnabled(alarms[selectedAlarmId].hour, alarms[selectedAlarmId].minute, true);
                transitionToState(AlarmState::DEFAULT);
                break;
            case 1:  // Disable
                setAlarmEnabled(alarms[selectedAlarmId].hour, alarms[selectedAlarmId].minute, false);
                transitionToState(AlarmState::DEFAULT);
                break;
            case 2:  // Delete
                deleteAlarm(alarms[selectedAlarmId].hour, alarms[selectedAlarmId].minute);
                transitionToState(AlarmState::DEFAULT);
                break;
        }
    });

    UIManager& uiManager = appManager.getUIManager();
    int optionListId = uiManager.addOrUpdateComponent(optionsList);
    activeComponents.push_back(optionListId);
}

void Alarm::backgroundActivity() {
    while(1) {
         // TODO: add alarm checks
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void Alarm::alarmCheckTask(void* params) {
    static_cast<Alarm*>(params)->backgroundActivity();
}


void Alarm::serializeAlarms() {
    std::string data;
    for (const auto& alarm : alarms) {
        data += std::to_string(alarm.hour) + ":" + std::to_string(alarm.minute) + " " + (alarm.enabled ? "1" : "0") + "\n";
    }
    FileManager& fileManager = appManager.getFileManager();
    fileManager.writeData("AlarmApp", "alarms.txt", data);
}

void Alarm::deserializeAlarms() {
    FileManager& fileManager = appManager.getFileManager();
    std::string data = fileManager.readData("AlarmApp", "alarms.txt");
    std::istringstream iss(data);
    std::string line;
    while (std::getline(iss, line)) {
        std::istringstream linestream(line);
        int hour, minute;
        char delim;
        bool enabled;
        linestream >> hour >> delim >> minute >> enabled;
        alarms.push_back({hour, minute, enabled});
    }
}