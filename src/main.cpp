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

#define LCD_ADDR 0x27
#define SDA_PIN  13
#define SCL_PIN  14
#define LCD_COLS 16
#define LCD_ROWS 2

extern "C" void app_main() {
    static UIManager uiManager;
    static TimeManager timeManager = TimeManager(uiManager);
    LCD_init(LCD_ADDR, SDA_PIN, SCL_PIN, LCD_COLS, LCD_ROWS);

    // add the only components available to the clock.
    uiManager.addOrUpdateComponent(ComponentID::Time, TextComponent("--:--:--", true));
    uiManager.addOrUpdateComponent(ComponentID::Title, TextComponent("A Watch", true));

    // Eventually we want this in a task manager or as part of each manager maybe?
    xTaskCreate(&UIManager::renderTask, "Rendering Task", 2048, &uiManager, 5, NULL);
    xTaskCreate(&TimeManager::timeTask, "Timing Task", 2048, &timeManager, 5, NULL);
}
