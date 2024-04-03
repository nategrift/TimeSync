#ifndef APPMANAGER_H
#define APPMANAGER_H

#include <vector>
#include <string>
#include <functional>
#include "IApp.h"
#include "InputManager.h"
#include "UIManager.h"
#include "TimeManager.h"
#include "FileManager.h"


class AppManager {
private:
    std::vector<std::pair<std::string, IApp*>> appRegistry;
    std::string openAppName;
    UIManager& uiManager;
    InputManager& inputManager;
    TimeManager& timeManager;
    FileManager& fileManager;

public:
    AppManager(UIManager& uiManager, InputManager& inputManager, TimeManager& timeManager, FileManager& fileManager) : 
        uiManager(uiManager), inputManager(inputManager), timeManager(timeManager), fileManager(fileManager) {}


    void registerApp(const std::string& name, IApp* app);
    void launchApp(const std::string& appName);
    void closeApp(const std::string& appName);
    IApp* getCurrentApp();
    std::vector<std::pair<std::string, IApp*>>& getAppRegistry();

    UIManager& getUIManager() {return uiManager;}  
    InputManager& getInputManager() {return inputManager;}
    TimeManager& getTimeManager() {return timeManager;}
    FileManager& getFileManager() {return fileManager;}

};

#endif // APPMANAGER_H
