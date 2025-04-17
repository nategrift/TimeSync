#include "AppManager.h"
#include "IApp.h"
#include "lvgl.h"
#include <string.h>
#include <algorithm>
#include "esp_log.h"
#include "FileManager.h"
#include "app_runtime.h"
static const char* TAG = "AppManager";

std::vector<AppConfig> AppManager::appRegistry = {};

void AppManager::loadAppsFromDisk() {
    char** files;
    int count;
    file_manager_get_files_in_directory("apps", &files, &count);
    for (int i = 0; i < count; i++) {
        // Check if this is a .config file
        if (strstr(files[i], ".config") != NULL) {
            char* data = file_manager_read_data("apps", files[i]);
            if (data) {
                ESP_LOGI("AppManager", "Loading app: %s", files[i]);

                AppConfig appConfig;
                char* line = strtok(data, "\n");
                while (line != nullptr) {
                    char* separator = strchr(line, '=');
                    if (separator != nullptr) {
                        *separator = '\0'; // Split the line at '='
                        const char* key = line;
                        const char* value = separator + 1;
                        
                        if (strcmp(key, "name") == 0) {
                            appConfig.name = value;
                        } else if (strcmp(key, "version") == 0) {
                            appConfig.version = value;
                        } else if (strcmp(key, "author") == 0) {
                            appConfig.author = value;
                        } else if (strcmp(key, "entry_point") == 0) {
                            // Extract directory from files[i] and combine with entry point
                            char* dirEnd = strrchr(files[i], '/');
                            if (dirEnd) {
                                std::string dir(files[i], dirEnd - files[i] + 1);
                                appConfig.entry_point = "apps/" + dir + value;
                            } else {
                                appConfig.entry_point = std::string("apps/") + value;
                            }
                        }
                    }
                    line = strtok(nullptr, "\n");
                }
                
                appRegistry.push_back(appConfig);

                free(data);
            }
        }
    }
}


void AppManager::launchApp(const std::string& appName) {
    ESP_LOGI("AppManager", "Launching %s", appName.c_str());
    // Find app with matching name in registry
    if (appRunning()) {
        closeLuaApp();
    }
    for (auto& app : appRegistry) {
        if (app.name == appName) {
            openLuaApp(&app);
            return;
        }
    }
    ESP_LOGE("AppManager", "App '%s' not found", appName.c_str());
}

void AppManager::closeApp(const std::string& appName) {
    ESP_LOGI("AppManager", "Closing %s", appName.c_str());
    closeLuaApp();
}



