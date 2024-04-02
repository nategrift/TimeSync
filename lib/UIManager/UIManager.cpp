#include <vector>
#include "UIManager.h"

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "HD44780.h"
#include <esp_log.h>
}

UIManager::UIManager() {

}

/**
 * Render function for task. Needs to be static so we call it statically then cast the instance of the UIManager to have the correct state
*/
void UIManager::renderTask(void *param) {
    auto *instance = static_cast<UIManager *>(param);
    instance->render();
}

void UIManager::addOrUpdateComponent(ComponentID id, const TextComponent& component) {
    components[id] = component;
    needsUpdate[id] = true;
}

void UIManager::updateComponentText(ComponentID id, const std::string& newText) {
    if (id >= 0 && id < components.size()) {
        components[id].text = newText;
        needsUpdate[id] = true;
    }
}

void UIManager::render() {
    while (true) {
        // Render each component based on the id
        bool freshScreen = false;
        for (auto& pair : components) {
            const auto& component = pair.second;
            if (component.visible && needsUpdate[pair.first]) {
                // only clear screen if there is an update
                if (freshScreen) {
                    LCD_home();
                    LCD_clearScreen();
                    freshScreen = false;
                }
                // set based on which element is added to screen first, we will change this to a better display later
                LCD_setCursor(0, static_cast<int>(pair.first));
                LCD_writeStr(const_cast<char*>(component.text.c_str()));
                ESP_LOGI("UIManager", "Displaying: %s", component.text.c_str());
                needsUpdate[pair.first] = false;
            }
        }

        // currently only 2fps
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}