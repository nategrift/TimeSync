#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "TextComponent.h"
#include "ComponentID.h"
#include <map>

class UIManager {
private:
    std::map<ComponentID, TextComponent> components;
    std::map<ComponentID, bool> needsUpdate;

public:
    UIManager();

    static void renderTask(void *param);
    void addOrUpdateComponent(ComponentID id, const TextComponent& component);
    void updateComponentText(ComponentID id, const std::string& newText);
    void render();
};
#endif // UI_MANAGER_H