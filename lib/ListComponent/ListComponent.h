#ifndef LIST_COMPONENT_H
#define LIST_COMPONENT_H

#include "IUIComponent.h"
#include <memory>
#include <vector>

class AppManager; // Forward declaration

class ListComponent: public IUIComponent {
public:
    ListComponent(AppManager& manager);
    ~ListComponent();

    ElementType getType() override { return ElementType::LIST; };

    void addItem(const std::shared_ptr<IUIComponent>& item);
    void navigate(int direction);

    void render(int yOffset) override;
private:
    AppManager& appManager;
    std::vector<std::shared_ptr<IUIComponent>> items;
    int currentIndex;
    int inputListenerId;
};

#endif // LIST_COMPONENT_H
