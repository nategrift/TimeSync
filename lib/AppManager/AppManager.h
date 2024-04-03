#ifndef APPMANAGER_H
#define APPMANAGER_H

#include <vector>
#include <string>
#include <functional>
#include "IApp.h"
#include "InputManager.h"
#include "UIManager.h"
#include "TimeManager.h"


class AppManager {
private:
    std::vector<std::pair<std::string, IApp*>> appRegistry;
    std::string openAppName;
    UIManager& uiManager;
    InputManager& inputManager;
    TimeManager& timeManager;

public:
    AppManager(UIManager& uiManager, InputManager& inputManager, TimeManager& timeManager) : 
        uiManager(uiManager), inputManager(inputManager), timeManager(timeManager) {}


    void registerApp(const std::string& name, IApp* app);
    void launchApp(const std::string& appName);
    void closeApp(const std::string& appName);
    IApp* getCurrentApp();
    std::vector<std::pair<std::string, IApp*>>& getAppRegistry();

    UIManager& getUIManager() {return uiManager;}  
    InputManager& getInputManager() {return inputManager;}
    TimeManager& getTimeManager() {return timeManager;}

};

#endif // APPMANAGER_H
