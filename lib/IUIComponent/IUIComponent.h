#ifndef IUI_COMPONENT_H
#define IUI_COMPONENT_H

enum class ElementType {
    GENERIC,
    LIST,
    TEXT
};


class IUIComponent {
private:

public:
    virtual ~IUIComponent() {}

    // we currently have a yOffset but once we get a 2d seen, include xOffset
    virtual void render(int yOffset) = 0;

    virtual ElementType getType() { return ElementType::GENERIC; };
};

#endif // IUI_COMPONENT_H