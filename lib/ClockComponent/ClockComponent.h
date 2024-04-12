#ifndef CLOCKCOMPONENT_H
#define CLOCKCOMPONENT_H

#include "IUIComponent.h"
#include "AppManager.h"
#include <functional>
#include <string>

class ClockComponent : public IUIComponent {
private:
    AppManager& appManager;
    int inputListenerId;
    int clockTime_UI;
    int clockLabel_UI;
    int hour;
    int minute;
    bool isAM;

    enum class State {
        HOUR,
        MINUTE,
        AMPM
    } state;

    std::function<void(int hour, int min, bool isAm)> onSelect;

    void incrementHour(int direction);
    void incrementMinute(int direction);
    void toggleAMPM();
    std::string getLabelForState();
    std::string getTimeString() const;

public:
    ClockComponent(AppManager& manager);
    virtual ~ClockComponent();

    void render(int yOffset) override;
    void cleanUp();
    void setOnSelectCallback(const std::function<void(int hour, int min, bool isAm)>& callback);
};

#endif // CLOCKCOMPONENT_H
