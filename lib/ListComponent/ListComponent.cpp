#include "ListComponent.h"
#include "AppManager.h"
#include "esp_log.h"

ListComponent::ListComponent(AppManager& manager) 
    : appManager(manager), currentIndex(0), inputListenerId(-1) {
    inputListenerId = appManager.getInputManager().addListener([this](InputEvent event) {
        ESP_LOGI("ListComponent", "Input");
        switch (event) {
            // case InputEvent::JOYSTICK_UP:
            //     navigate(-1);
            //     break;
            // case InputEvent::JOYSTICK_DOWN:
            //     navigate(1);
            //     break;
            // case InputEvent::BUTTON_PRESS:
            //     if (onSelect && items.contains(currentIndex)) {
            //         onSelect(currentIndex);
            //     }
            //     break;
            default:
                break;
        }
        return true;
    });
    
}

ListComponent::~ListComponent() {
    if (inputListenerId != -1) {
        appManager.getInputManager().removeListener(inputListenerId);
        inputListenerId = -1;
    }
}

void ListComponent::addItem(int id, const std::shared_ptr<IUIComponent>& item) {
    items[id] = item;
}

void ListComponent::render(int yOffset) {
    if (items.empty()) return;

    int count = 0;
    auto it = items.find(currentIndex);
    if (it == items.end()) {
        it = items.begin();
        currentIndex = it->first;
    }

    auto startKey = it->first;

    while (count < 2) {
        if (it == items.end()) {
            it = items.begin();
            // if we loop back to the start, exit
            if (it->first == startKey) break;
        }

        it->second->render(yOffset + count);
        ++count;
        ++it;
    }
}



void ListComponent::navigate(int direction) {
    if (items.empty()) return;
    
    auto it = items.find(currentIndex);
    if (it != items.end()) {
        if (direction > 0) {
            ++it;
            if (it == items.end()) {
                it = items.begin();
            }
        } else {
            if (it == items.begin()) {
                it = items.end();
            }
            --it;
        }
        currentIndex = it->first;
    } else {
        currentIndex = direction > 0 ? items.begin()->first : items.rbegin()->first;
    }
}


void ListComponent::setOnSelectCallback(const std::function<void(int)>& callback) {
    onSelect = callback;
}