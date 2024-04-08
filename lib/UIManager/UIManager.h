#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "IUIComponent.h"
#include <string>
#include <map>
#include <memory>
#include <vector>

#define LCD_ADDR 0x27
#define SDA_PIN  13
#define SCL_PIN  14
#define LCD_COLS 16
#define LCD_ROWS 2

class UIManager {
private:
    using ComponentID = int;
    std::map<ComponentID, std::shared_ptr<IUIComponent>> components;
    std::map<ComponentID, bool> needsUpdate;
    ComponentID nextComponentId = 1;
    std::mutex componentsMutex;
    std::vector<ComponentID> deletionQueue;

public:
    UIManager();

    static void renderTask(void *param);
    bool hasComponent(ComponentID id);
    ComponentID addOrUpdateComponent(std::shared_ptr<IUIComponent> component);
    void updateComponentText(ComponentID id, const std::string& newText);
    void deleteComponent(ComponentID id);
    void render();
    void renderLoop();
    void processDeletionQueue();
};


#endif // UI_MANAGER_H
