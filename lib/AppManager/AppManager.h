#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include <string>
#include <vector>
#include <string>


struct AppConfig {
    std::string name;
    std::string version;
    std::string author;
    std::string entry_point;
    std::string primary_color;  // Hex color like "E9444C"
};

class AppManager {
private:
    static std::vector<AppConfig> appRegistry;

public:
    static void launchApp(const std::string& appName);
    static void closeApp(const std::string& appName);
    static void loadAppsFromDisk();
    static std::vector<AppConfig>& getAppRegistry() { return AppManager::appRegistry; }
};

#endif // APP_MANAGER_H
