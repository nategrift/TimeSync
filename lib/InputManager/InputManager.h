#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <map>
#include <functional>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc_cal.h"
#include "TouchDriver.h"

// Current types of input events right now
enum class InputEvent {
    BUTTON_LONG_PRESS
};


using InputListener = std::function<bool(InputEvent)>;

class InputManager {
private:
    std::map<int, InputListener> listeners;
    int nextId = 0; 

    TouchDriver& touchDriver;


    void pollInputs();
    void notifyListeners(InputEvent event);

public:
    InputManager(TouchDriver& touchDriver);

    static void inputTask(void* arg);
    
    int addListener(const InputListener& listener);
    void removeListener(int id);
};

#endif // INPUTMANAGER_H