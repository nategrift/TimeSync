#include <vector>
#include "UIManager.h"

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "HD44780.h"
#include <esp_log.h>
}

UIManager::UIManager() {
    LCD_init(LCD_ADDR, SDA_PIN, SCL_PIN, LCD_COLS, LCD_ROWS);
}

/**
 * Render function for task. Needs to be static so we call it statically then cast the instance of the UIManager to have the correct state
*/
void UIManager::renderTask(void *param) {
    auto *instance = static_cast<UIManager *>(param);
    instance->render();
}

bool UIManager::hasComponent(ComponentID id) {
    return components.contains(id);
}
int UIManager::addOrUpdateComponent(const TextComponent& component) {
    ComponentID id = nextComponentId++;
    components[id] = component;
    needsUpdate[id] = true;
    return id;
}

void UIManager::updateComponentText(ComponentID id, const std::string& newText) {
    if (id > 0 && components.contains(id)) {
        // make sure we only update if needed
        if (components[id].text != newText) {
            components[id].text = newText;
            needsUpdate[id] = true;
        }
    } else {
        ESP_LOGI("UIManager", "ERROR: Unable to update text for %d, it doesn't exist", id);
    }
}

void UIManager::render() {
    while (true) {
        // Render each component based on the id
        int currentRow = 0;
        for (auto& pair : components) {
            const auto& component = pair.second;
            // make sure we want to render the component, and make sure that we have room on screen
            if (component.visible && needsUpdate[pair.first] && currentRow < 2) {
                // set based on which element is added to screen first, we will change this to a better display later
                LCD_setCursor(0, currentRow);

                // clear the screen first
                std::string clearLine(LCD_COLS, ' '); // Create a string of spaces the length of LCD_COLS
                LCD_writeStr(const_cast<char*>(clearLine.c_str()));

                LCD_setCursor(0, currentRow);
                LCD_writeStr(const_cast<char*>(component.text.c_str()));
                ESP_LOGI("UIManager", "Displaying: %s", component.text.c_str());
                needsUpdate[pair.first] = false;
                currentRow++;
            }
        }

        // currently only 2fps
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void UIManager::deleteComponent(ComponentID id) {
    if (components.contains(id)) {
        components.erase(id);
        needsUpdate.erase(id);
        ESP_LOGI("UIManager", "Component %d deleted successfully", id);
    } else {
        ESP_LOGI("UIManager", "ERROR: Unable to delete component %d, it doesn't exist", id);
    }
}
