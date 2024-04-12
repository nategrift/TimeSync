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
    uint32_t buttonPressStartTime = 0;
    uint32_t longPressDuration = 500; // ms
    bool isButtonPressed = false;

    while (true) {
        int joystickValue = adc1_get_raw(joystickChannel);

        if (joystickValue > JOYSTICK_MIDDLE_THRESHOLD + JOYSTICK_PRECISION) {
            notifyListeners(InputEvent::JOYSTICK_DOWN);
            vTaskDelay(pdMS_TO_TICKS(600));
        } else if (joystickValue < JOYSTICK_MIDDLE_THRESHOLD - JOYSTICK_PRECISION) {
            notifyListeners(InputEvent::JOYSTICK_UP);
            vTaskDelay(pdMS_TO_TICKS(600));
        }
        
        int buttonState = gpio_get_level(buttonPin);
        if (buttonState == 0) {
            if (!isButtonPressed) {
                buttonPressStartTime = xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
                ESP_LOGI("Input", "pressed button down");
                isButtonPressed = true;
            }
        } else {
            if (isButtonPressed) {
                ESP_LOGI("Input", "lifted button up");
                uint32_t currentTime = xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
                if (currentTime - buttonPressStartTime > longPressDuration) {
                    notifyListeners(InputEvent::BUTTON_LONG_PRESS);
                } else {
                    notifyListeners(InputEvent::BUTTON_PRESS);
                }
                isButtonPressed = false;
                buttonPressStartTime = 0;
            }
        }

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
