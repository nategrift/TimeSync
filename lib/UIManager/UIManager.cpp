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

#include <mutex>


UIManager::UIManager() {
    LCD_init(LCD_ADDR, SDA_PIN, SCL_PIN, LCD_COLS, LCD_ROWS);
}

void UIManager::renderTask(void *param) {
    auto *instance = static_cast<UIManager *>(param);
    instance->renderLoop();
}

bool UIManager::hasComponent(ComponentID id) {
    return components.find(id) != components.end();
}

UIManager::ComponentID UIManager::addOrUpdateComponent(std::shared_ptr<IUIComponent> component) {
    std::lock_guard<std::mutex> lock(componentsMutex);
    ComponentID id = nextComponentId++;
    components[id] = component;
    needsUpdate[id] = true;
    return id;
}


void UIManager::updateComponentText(ComponentID id, const std::string& newText) {
    std::lock_guard<std::mutex> lock(componentsMutex);
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

void UIManager::deleteComponent(ComponentID id) {
    std::lock_guard<std::mutex> lock(componentsMutex);
    if (hasComponent(id)) {
        deletionQueue.push_back(id);
        ESP_LOGE("UIManager", "Added componenet %d to delete queue", id);
    }
}
void UIManager::processDeletionQueue() {
    std::lock_guard<std::mutex> lock(componentsMutex);
    for (auto id : deletionQueue) {
        components.erase(id);
        needsUpdate.erase(id);
    }
    deletionQueue.clear();
}


void UIManager::render() {
    ESP_LOGI("UIManager", "Render func");

    // copy the components so we don't hold up the mutex lock
    decltype(components) componentsCopy;
    {
        std::lock_guard<std::mutex> lock(componentsMutex);
        componentsCopy = components;
    }

    int currentRow = 0;
    LCD_clearScreen();
    for (auto& pair : components) {
        ESP_LOGI("UIManager", "Rending component with %d", pair.first);
        auto component = pair.second;
        if (component != nullptr) {
            component->render(currentRow);
            currentRow++;
        }
    }
}



void UIManager::renderLoop() {
    while (true) {
        processDeletionQueue();

        render();

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
