#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include <string>
#include <vector>
#include <string>
#include <functional>
#include "IApp.h"
#include "InputManager.h"
#include "FileManager.h"
#include "BatteryManager.h"
#include "utility"
#include "TouchDriver.h"
#include "LVGLMutex.h"

class IApp;

class AppManager {
private:
    std::vector<std::pair<std::string, IApp*>> appRegistry;
    std::string openAppName;
    TouchDriver& touchDriver;
    FileManager& fileManager;
    InputManager& inputManager;
    BatteryManager& batteryManager;

public:
    AppManager(TouchDriver& touchDriver, FileManager& fileManager, InputManager& inputManager, BatteryManager& batteryManager) : 
        touchDriver(touchDriver), fileManager(fileManager), inputManager(inputManager), batteryManager(batteryManager) {}

    void registerApp(IApp* app);
    void launchApp(const std::string& appName);
    void launchAppAtIndex(size_t index);
    void launchNextApp();
    void launchPreviousApp();
    void closeApp(const std::string& appName);
    IApp* getCurrentApp();
    std::vector<std::pair<std::string, IApp*>>& getAppRegistry();

    InputManager& getInputManager() {return inputManager;}
    FileManager& getFileManager() {return fileManager;}
    BatteryManager& getBatteryManager() {return batteryManager;}

};

#endif // APP_MANAGER_H
