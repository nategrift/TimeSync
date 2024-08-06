#include "AppManager.h"
#include "IApp.h"
#include "lvgl.h"
#include <algorithm>
#include "LvglMutex.h"

void AppManager::registerApp(IApp* app) {
    appRegistry.push_back({app->getAppName(), app});
}

void AppManager::launchApp(const std::string& appName) {
    IApp* currentApp = getCurrentApp();
    for (auto& appPair : appRegistry) {
        if (appPair.first == appName) {
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

    size_t currentIndex = std::distance(appRegistry.begin(), std::find_if(appRegistry.begin(), appRegistry.end(),
                                                                         [this](const std::pair<std::string, IApp*>& appPair) {
                                                                             return appPair.first == openAppName;
                                                                         }));

    size_t nextIndex = (currentIndex + 1) % appRegistry.size();
    launchAppAtIndex(nextIndex);
}

void AppManager::launchPreviousApp() {
    if (appRegistry.empty()) return;

    size_t currentIndex = std::distance(appRegistry.begin(), std::find_if(appRegistry.begin(), appRegistry.end(),
                                                                         [this](const std::pair<std::string, IApp*>& appPair) {
                                                                             return appPair.first == openAppName;
                                                                         }));

    size_t previousIndex = (currentIndex + appRegistry.size() - 1) % appRegistry.size();
    launchAppAtIndex(previousIndex);
}

void AppManager::closeApp(const std::string& appName) {
    if (this->openAppName == appName) {
        IApp* currentApp = getCurrentApp();
        if (currentApp) {
            currentApp->close();
            this->openAppName.clear();
        }
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
