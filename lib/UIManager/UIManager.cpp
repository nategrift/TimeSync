#include <vector>
#include <map>
#include <memory>
#include "IUIComponent.h"
#include "TextComponent.h"
#include <typeinfo>
#include "UIManager.h"

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "HD44780.h"
#include <esp_log.h>
#include "esp_task_wdt.h"
}

UIManager::UIManager() {
    LCD_init(LCD_ADDR, SDA_PIN, SCL_PIN, LCD_COLS, LCD_ROWS);
}

void UIManager::renderTask(void *param) {
    auto *instance = static_cast<UIManager *>(param);
    if (instance != nullptr) {
        instance->render();
    }
}

bool UIManager::hasComponent(ComponentID id) {
    return components.find(id) != components.end();
}

int UIManager::addOrUpdateComponent(std::shared_ptr<IUIComponent> component) {
    ComponentID id = nextComponentId++;
    components[id] = component;
    needsUpdate[id] = true;
    return id;
}

void UIManager::updateComponentText(ComponentID id, const std::string& newText) {
    if (id > 0 && hasComponent(id)) {
        auto& component = components[id];
        if (component && component->getType() == ElementType::TEXT) {
            auto textComponent = std::static_pointer_cast<TextComponent>(component);
            if (textComponent != nullptr) {
                textComponent->text = newText;
                needsUpdate[id] = true;
            }
        } else {
            ESP_LOGI("UIManager", "Component with id %d is not a Text Component", id);
        }
    } else {
        ESP_LOGI("UIManager", "ERROR: Unable to update text for %d, it doesn't exist", id);
    }
}

void UIManager::render() {
    while (true) {
        int currentRow = 0;
        LCD_clearScreen();
        for (auto& pair : components) {
            auto component = pair.second;
            // make sure we want to render the component
            if (component != nullptr) {
                component->render(currentRow);
                // needsUpdate[pair.first] = false;
                currentRow++;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void UIManager::deleteComponent(ComponentID id) {
    if (hasComponent(id)) {
        components.erase(id);
        needsUpdate.erase(id);
        ESP_LOGI("UIManager", "Component %d deleted successfully", id);
    } else {
        ESP_LOGI("UIManager", "ERROR: Unable to delete component %d, it doesn't exist", id);
    }
}
