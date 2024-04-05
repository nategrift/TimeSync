#ifndef TEXT_COMPONENT_H
#define TEXT_COMPONENT_H


#include "IUIComponent.h"
#include <string>

struct TextComponent: public IUIComponent {
    std::string text; 

    TextComponent() : text("") {}

    TextComponent(std::string txt)
    : text(txt) {}

     ElementType getType() override { return ElementType::TEXT; };

    void render(int yOffset) override;
};

#endif // TEXT_COMPONENT_H
