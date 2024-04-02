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

#define LCD_ADDR 0x27
#define SDA_PIN  13
#define SCL_PIN  14
#define LCD_COLS 16
#define LCD_ROWS 2

// Define GPIO pins for the joystick and button
#define JOYSTICK_CHANNEL ADC1_CHANNEL_7
#define BUTTON_PIN GPIO_NUM_12

extern "C" void app_main() {
    static UIManager uiManager;
    static TimeManager timeManager(uiManager);
    static InputManager inputManager(JOYSTICK_CHANNEL, BUTTON_PIN);

    LCD_init(LCD_ADDR, SDA_PIN, SCL_PIN, LCD_COLS, LCD_ROWS);

    // Add the only components available to the clock.
    uiManager.addOrUpdateComponent(ComponentID::Time, TextComponent("--:--:--", true));
    uiManager.addOrUpdateComponent(ComponentID::Title, TextComponent("A Watch", true));

    // Create tasks for rendering and time management
    xTaskCreate(&UIManager::renderTask, "Rendering Task", 2048, &uiManager, 5, NULL);
    xTaskCreate(&TimeManager::timeTask, "Timing Task", 2048, &timeManager, 5, NULL);
    xTaskCreate(&InputManager::inputTask, "inputTask", 2048, &inputManager, 4, NULL);

    // TEST cases for adding input events
    inputManager.addListener([](InputEvent event) {
        switch (event) {
            case InputEvent::JOYSTICK_UP:
                ESP_LOGI("InputManager", "Joystick moved up");
                break;
            case InputEvent::JOYSTICK_DOWN:
                ESP_LOGI("InputManager", "Joystick moved down");
                break;
            case InputEvent::BUTTON_CLICK:
                ESP_LOGI("InputManager", "Button clicked");
                break;
        }
    });
}
