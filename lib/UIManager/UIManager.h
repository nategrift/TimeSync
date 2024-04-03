#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "TextComponent.h"
#include <map>

#define LCD_ADDR 0x27
#define SDA_PIN  13
#define SCL_PIN  14
#define LCD_COLS 16
#define LCD_ROWS 2

class UIManager {
private:
    using ComponentID = int;
    std::map<ComponentID, TextComponent> components;
    std::map<ComponentID, bool> needsUpdate;
    ComponentID nextComponentId = 1;

public:

    UIManager();

    static void renderTask(void *param);
    bool hasComponent(ComponentID id);
    ComponentID addOrUpdateComponent(const TextComponent& component);
    void updateComponentText(ComponentID id, const std::string& newText);
    void deleteComponenet(ComponentID id);
    void render();
};
#endif // UI_MANAGER_H