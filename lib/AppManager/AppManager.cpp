#include "AppManager.h"

void AppManager::registerApp(const std::string& name, IApp* app) {
    appRegistry.push_back({name, app});
}

void AppManager::launchApp(const std::string& appName) {
    
    for (auto& appPair : appRegistry) {
        if (appPair.first == appName) {
            // close the previous one
            IApp* currentApp = getCurrentApp();
            if (currentApp != nullptr) {
                currentApp->close();
            }

            appPair.second->launch();
            this->openAppName = appName;

            break;
        }
    }
}

void AppManager::closeApp(const std::string& appName) {
    if (this->openAppName == appName) {
        IApp* currentApp = getCurrentApp();
        currentApp->close();
    }
}

IApp* AppManager::getCurrentApp() {
    if (!this->openAppName.empty()) {
        for (auto& appPair : appRegistry) {
            if (appPair.first == this->openAppName) {
                return appPair.second;
            }
        }
    }
    return nullptr; // No app is open
}

std::vector<std::pair<std::string, IApp*>>& AppManager::getAppRegistry() {
    return appRegistry;
}
