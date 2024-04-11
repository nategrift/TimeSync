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

void AppManager::launchAppAtIndex(size_t index) {
    if (index < appRegistry.size()) {
        auto& appPair = appRegistry[index];
        launchApp(appPair.first);
    }
}

void AppManager::launchNextApp() {
    if (appRegistry.empty()) return;

    size_t currentIndex = 0;
    for (size_t i = 0; i < appRegistry.size(); i++) {
        if (appRegistry[i].first == openAppName) {
            currentIndex = i;
            break;
        }
    }

    size_t nextIndex = (currentIndex + 1) % appRegistry.size();
    launchAppAtIndex(nextIndex);
}

void AppManager::launchPreviousApp() {
    if (appRegistry.empty()) return;

    size_t currentIndex = 0;
    for (size_t i = 0; i < appRegistry.size(); i++) {
        if (appRegistry[i].first == openAppName) {
            currentIndex = i;
            break;
        }
    }

    size_t previousIndex = (currentIndex + appRegistry.size() - 1) % appRegistry.size();
    launchAppAtIndex(previousIndex);
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
