extern "C" {
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "sdkconfig.h"
#include "HD44780.h"
}

#include "UIManager.h"
#include "TimeManager.h"
#include "TextComponent.h"
#include "InputManager.h"


#include "AppManager.h"
// START APPS

#include "Alarm.h"
#include "Clock.h"

// END APPS

// Define GPIO pins for the joystick and button
#define JOYSTICK_CHANNEL ADC1_CHANNEL_7
#define BUTTON_PIN GPIO_NUM_12

extern "C" void app_main() {
    static UIManager uiManager;
    static TimeManager timeManager;
    static InputManager inputManager(JOYSTICK_CHANNEL, BUTTON_PIN);

    static AppManager appManager(uiManager, inputManager, timeManager);

    Clock* clockApp = new Clock(appManager);
    Alarm* alarmApp = new Alarm(appManager);

    appManager.registerApp("Clock", clockApp);
    appManager.registerApp("Alarm", alarmApp);

    appManager.launchApp("Clock");

    // Create tasks for rendering and time management
    xTaskCreate(&UIManager::renderTask, "Rendering Task", 2048, &uiManager, 5, NULL);
    xTaskCreate(&TimeManager::timeTask, "Timing Task", 2048, &timeManager, 5, NULL);
    xTaskCreate(&InputManager::inputTask, "Input Task", 4096, &inputManager, 4, NULL);

    // TEST cases for adding input events
    inputManager.addListener([](InputEvent event) {
        switch (event) {
            case InputEvent::JOYSTICK_UP:
                ESP_LOGI("InputManager", "Joystick moved up");
                appManager.launchApp("Clock");
                break;
            case InputEvent::JOYSTICK_DOWN:
                ESP_LOGI("InputManager", "Joystick moved down");
                appManager.launchApp("Alarm");
                break;
            case InputEvent::BUTTON_CLICK:
                ESP_LOGI("InputManager", "Button clicked");
                break;
        }
    });
}
