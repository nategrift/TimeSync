#ifndef LIST_COMPONENT_H
#define LIST_COMPONENT_H

#include "IUIComponent.h"
#include <memory>
#include <map>
#include <functional>


class AppManager;

class ListComponent: public IUIComponent {
public:
    ListComponent(AppManager& manager);
    ~ListComponent();

    ElementType getType() override { return ElementType::LIST; };

    void addItem(int id, const std::shared_ptr<IUIComponent>& item);
    void navigate(int direction);

    void render(int yOffset) override;


    void setOnSelectCallback(const std::function<void(int)>& callback);
private:
    std::function<void(int)> onSelect;
    AppManager& appManager;
    std::map<int, std::shared_ptr<IUIComponent>> items;
    int currentIndex;
    int inputListenerId;
};

#endif // LIST_COMPONENT_H
