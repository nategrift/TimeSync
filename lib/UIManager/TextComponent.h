#ifndef TEXT_COMPONENT_H
#define TEXT_COMPONENT_H

#include <string>

struct TextComponent {
    std::string text; 
    bool visible;

    TextComponent() : text(""), visible(false) {}

    TextComponent(std::string txt, bool vis)
    : text(txt), visible(vis) {}
};

#endif // TEXT_COMPONENT_H
