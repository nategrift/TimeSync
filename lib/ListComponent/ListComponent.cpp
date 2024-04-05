#include "ListComponent.h"
#include "AppManager.h"

ListComponent::ListComponent(AppManager& manager) 
    : appManager(manager), currentIndex(0), inputListenerId(-1) {
    // Register for joystick input
    inputListenerId = appManager.getInputManager().addListener([this](InputEvent event) {
        switch (event) {
            case InputEvent::JOYSTICK_UP:
                navigate(-1);
                break;
            case InputEvent::JOYSTICK_DOWN:
                navigate(1);
                break;
            default:
                break;
        }
    });
}

ListComponent::~ListComponent() {
    // Unregister the input listener
    if (inputListenerId != -1) {
        appManager.getInputManager().removeListener(inputListenerId);
        inputListenerId = -1;
    }
}

void ListComponent::addItem(const std::shared_ptr<IUIComponent>& item) {
    items.push_back(item);
}

void ListComponent::render(int yOffset) {
    // Render only two items starting from currentIndex
    for (int i = 0; i < 2; ++i) {
        int itemIndex = (currentIndex + i) % items.size();
        if (itemIndex < items.size()) {
            items[itemIndex]->render(i);
        }
    }
}

void ListComponent::navigate(int direction) {
    // Update currentIndex to show next/previous items
    currentIndex += direction;
    if (currentIndex < 0) {
        currentIndex = items.size() - 1;
    } else if (currentIndex >= items.size()) {
        currentIndex = 0;
    }
}
