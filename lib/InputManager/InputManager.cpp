#include <vector>
#include <functional>
#include "driver/gpio.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>

#include "InputManager.h"

// It really should be 4096 / 2 however the hardware we have the middle is actually 3090.
constexpr uint32_t JOYSTICK_MIDDLE_THRESHOLD = 3090;
const uint32_t JOYSTICK_PRECISION = 500;

InputManager::InputManager(adc1_channel_t joystickChannel, gpio_num_t btnPin) 
    : joystickChannel(joystickChannel), buttonPin(btnPin) {
    // ADC for joystick read
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(joystickChannel, ADC_ATTEN_DB_12);

    // GPIO for button press
    gpio_set_direction(buttonPin, GPIO_MODE_INPUT);
    gpio_pullup_en(buttonPin);
    gpio_pulldown_dis(buttonPin);
}

void InputManager::inputTask(void* arg) {
    auto* instance = static_cast<InputManager*>(arg);
    instance->pollInputs();
}

void InputManager::pollInputs() {
    while (true) {
        int joystickValue = adc1_get_raw(joystickChannel);

        // Debounce delay to only register events after a delay
        // TODO: Eventually we should turn this into a timeout for each, so that we can handle multiple events of different events in that debouce time
        int debouceDelay = 600;

        if (joystickValue > JOYSTICK_MIDDLE_THRESHOLD + JOYSTICK_PRECISION) {
            notifyListeners(InputEvent::JOYSTICK_UP);
            vTaskDelay(pdMS_TO_TICKS(debouceDelay));
        } else if (joystickValue < JOYSTICK_MIDDLE_THRESHOLD - JOYSTICK_PRECISION) {
            notifyListeners(InputEvent::JOYSTICK_DOWN);
            vTaskDelay(pdMS_TO_TICKS(debouceDelay));
        }

        if (gpio_get_level(buttonPin) == 0) {
            notifyListeners(InputEvent::BUTTON_CLICK);
            vTaskDelay(pdMS_TO_TICKS(debouceDelay));
        }

        // we don't need to read very often, are hardware isn't fast enough to matter
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

void InputManager::notifyListeners(InputEvent event) {
    for (auto& listener : listeners) {
        listener.second(event);
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
