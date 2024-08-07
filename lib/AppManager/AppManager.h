#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include <string>
#include <vector>
#include <string>
#include <functional>
#include "IApp.h"
#include "InputManager.h"
#include "UIManager.h"
#include "TimeManager.h"
#include "FileManager.h"
#include "BatteryManager.h"
#include "utility"
#include "TouchDriver.h"

class IApp;

class AppManager {
private:
    std::vector<std::pair<std::string, IApp*>> appRegistry;
    std::string openAppName;
    TouchDriver& touchDriver;
    UIManager& uiManager;
    FileManager& fileManager;
    TimeManager& timeManager;
    InputManager& inputManager;
    BatteryManager& batteryManager;

public:
    AppManager(TouchDriver& touchDriver, UIManager& uiManager, FileManager& fileManager, TimeManager& timeManager, InputManager& inputManager, BatteryManager& batteryManager) : 
        touchDriver(touchDriver), uiManager(uiManager), fileManager(fileManager), timeManager(timeManager), inputManager(inputManager), batteryManager(batteryManager) {}

    void registerApp(const std::string& name, IApp* app);
    void launchApp(const std::string& appName);
    void launchAppAtIndex(size_t index);
    void launchNextApp();
    void launchPreviousApp();
    void closeApp(const std::string& appName);
    IApp* getCurrentApp();
    std::vector<std::pair<std::string, IApp*>>& getAppRegistry();

    UIManager& getUIManager() {return uiManager;}  
    InputManager& getInputManager() {return inputManager;}
    FileManager& getFileManager() {return fileManager;}
    TimeManager& getTimeManager() {return timeManager;}
    BatteryManager& getBatteryManager() {return batteryManager;}

};

#endif // APP_MANAGER_H
