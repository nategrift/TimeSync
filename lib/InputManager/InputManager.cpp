#include <vector>
#include <functional>
#include "driver/gpio.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>

#include "InputManager.h"

InputManager::InputManager(TouchDriver& touch) 
    : touchDriver(touch) {}


void InputManager::inputTask(void* arg) {
    auto* instance = static_cast<InputManager*>(arg);
    instance->pollInputs();
}
void InputManager::pollInputs() {
    uint32_t buttonPressStartTime = 0;
    uint32_t longPressDuration = 500; // ms
    bool isButtonPressed = false;

    while (true) {
        // TouchData touch = touchDriver.getTouchCoordinates();
        // if (touch.touch_detected) {
        //     if (!isButtonPressed) {
        //         buttonPressStartTime = xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
        //         isButtonPressed = true;
        //     }
        // } else {
        //     if (isButtonPressed) {
        //         uint32_t currentTime = xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
        //         if (currentTime - buttonPressStartTime > longPressDuration) {
        //             notifyListeners(InputEvent::BUTTON_LONG_PRESS);
        //         }
        //         isButtonPressed = false;
        //         buttonPressStartTime = 0;
        //     }
        // }

        vTaskDelay(pdMS_TO_TICKS(40));
    }
}


void InputManager::notifyListeners(InputEvent event) {
    for (auto& listener : listeners) {
        // if we return true then do not continue
        if (listener.second(event)) {
            break;
        }
    }
}

int InputManager::addListener(const InputListener& listener) {
        int id = nextId++;
        listeners[id] = listener;
        return id;
    }

void InputManager::removeListener(int id) {
    listeners.erase(id);
}
