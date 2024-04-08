#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <map>
#include <functional>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc_cal.h"

// Current types of input events right now
enum class InputEvent {
    JOYSTICK_UP,
    JOYSTICK_DOWN,
    BUTTON_PRESS,
    BUTTON_LONG_PRESS
};


using InputListener = std::function<void(InputEvent)>;

class InputManager {
private:
    adc1_channel_t joystickChannel;
    gpio_num_t buttonPin;

    std::map<int, InputListener> listeners;
    int nextId = 0; 


    void pollInputs();
    void notifyListeners(InputEvent event);

public:
    InputManager(adc1_channel_t joystickChannel, gpio_num_t btnPin);

    static void inputTask(void* arg);
    
    int addListener(const InputListener& listener);
    void removeListener(int id);
};

#endif // INPUTMANAGER_H