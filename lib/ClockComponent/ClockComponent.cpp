#include "ClockComponent.h"
#include "esp_log.h"
#include <stdio.h>
#include "TextComponent.h"

ClockComponent::ClockComponent(AppManager& manager) : appManager(manager), inputListenerId(-1), clockTime_UI(-1), clockLabel_UI(-1), hour(12), minute(0), isAM(true), state(State::HOUR) {
    UIManager& uiManager = appManager.getUIManager();
    auto clockComponent = std::make_shared<TextComponent>(getTimeString());
    clockTime_UI = uiManager.addOrUpdateComponent(clockComponent);
    auto clockLabelComponent = std::make_shared<TextComponent>(getLabelForState());
    clockLabel_UI = uiManager.addOrUpdateComponent(clockLabelComponent);

    inputListenerId = appManager.getInputManager().addListener([this](InputEvent event) {
        ESP_LOGI("ClockComponent", "Input");
        switch (event) {
            case InputEvent::JOYSTICK_UP:
                if (state == State::HOUR) incrementHour(1);
                else if (state == State::MINUTE) incrementMinute(1);
                else if (state == State::AMPM) toggleAMPM();
                break;
            case InputEvent::JOYSTICK_DOWN:
                if (state == State::HOUR) incrementHour(-1);
                else if (state == State::MINUTE) incrementMinute(-1);
                else if (state == State::AMPM) toggleAMPM();
                break;
            case InputEvent::BUTTON_PRESS:
                if (state == State::HOUR) state = State::MINUTE;
                else if (state == State::MINUTE) state = State::AMPM;
                else if (state == State::AMPM) {
                    state = State::HOUR;
                    if (onSelect != nullptr) onSelect(hour, minute, isAM);
                }
                break;
            default:
                break;
        }
        return true;
    });
}

ClockComponent::~ClockComponent() {
}

void ClockComponent::incrementHour(int direction) {
    hour = (((hour - 1) + direction + 12) % 12) + 1;
}

void ClockComponent::incrementMinute(int direction) {
    minute = (minute + direction + 60) % 60;
}

void ClockComponent::toggleAMPM() {
    isAM = !isAM;
}

std::string ClockComponent::getLabelForState() {
    switch (state)
    {
    case State::HOUR: return "+/- Hour";
    case State::MINUTE: return "+/- Minute";
    case State::AMPM: return "Set AM/PM";
    default:
        break;
    }
    return "";
}

std::string ClockComponent::getTimeString() const {
    char buffer[10];
    sprintf(buffer, "%02d:%02d %s", hour, minute, isAM ? "AM" : "PM");
    return std::string(buffer);
}

void ClockComponent::render(int yOffset) {
    UIManager& uiManager = appManager.getUIManager();
    uiManager.updateComponentText(clockTime_UI, getTimeString());
    uiManager.updateComponentText(clockLabel_UI, getLabelForState());
}

void ClockComponent::cleanUp() {
    UIManager& uiManager = appManager.getUIManager();
    uiManager.deleteComponent(clockTime_UI);
    uiManager.deleteComponent(clockLabel_UI);

    if (inputListenerId != -1) {
        appManager.getInputManager().removeListener(inputListenerId);
        inputListenerId = -1;
    }
}

void ClockComponent::setOnSelectCallback(const std::function<void(int, int, bool)>& callback) {
    onSelect = callback;
}
