#include "ConfigManager.h"
#include <sstream>
#include "esp_log.h"

static const char *TAG = "ConfigManager";

// Static member initialization
std::map<std::string, std::map<std::string, std::string>> ConfigManager::configMap;
std::map<std::string, std::map<std::string, std::string>> ConfigManager::defaultConfigMap = {
    {"General", {{"ScreenTimeout", "30"}, {"Time", "02:15:30"}, {"Date", "2024-08-01"}, {"Brightness", "10"}, {"Volume", "5"}, {"Mute", "1"}}},

};
std::string ConfigManager::configFileName;
FileManager* ConfigManager::fileManager = nullptr;

// Initialize the ConfigManager with the file manager and file name
void ConfigManager::init(FileManager& fm, const std::string& filename) {
    fileManager = &fm;
    configFileName = filename;
    deserializeConfig();
}

// Get a configuration value as a string
std::string ConfigManager::getConfigString(const std::string& group, const std::string& key) {
    if (hasConfigValue(group, key)) {
        return configMap[group][key];
    } else if (defaultConfigMap.find(group) != defaultConfigMap.end() && 
               defaultConfigMap[group].find(key) != defaultConfigMap[group].end()) {
        // ESP_LOGW(TAG, "Using default config value for group: %s, key: %s", group.c_str(), key.c_str());
        return defaultConfigMap[group][key];
    }
    ESP_LOGW(TAG, "Config value not found and no default available for group: %s, key: %s", group.c_str(), key.c_str());
    return "";
}

// Get a configuration value as an integer
int ConfigManager::getConfigInt(const std::string& group, const std::string& key) {
    if (hasConfigValue(group, key)) {
        return std::stoi(configMap[group][key]);
    } else if (defaultConfigMap.find(group) != defaultConfigMap.end() && 
               defaultConfigMap[group].find(key) != defaultConfigMap[group].end()) {
        // ESP_LOGW(TAG, "Using default config value for group: %s, key: %s", group.c_str(), key.c_str());
        return std::stoi(defaultConfigMap[group][key]);
    }
    ESP_LOGW(TAG, "Config value not found and no default available for group: %s, key: %s", group.c_str(), key.c_str());
    return 0;
}

// Check if a configuration value exists
bool ConfigManager::hasConfigValue(const std::string& group, const std::string& key) {
    return configMap.find(group) != configMap.end() && configMap[group].find(key) != configMap[group].end();
}

// Set a configuration value as a string
bool ConfigManager::setConfigString(const std::string& group, const std::string& key, const std::string& value) {
    if (!hasConfigValue(group, key) || configMap[group][key] != value) {
        configMap[group][key] = value;
        serializeConfig();
        return true;
    }
    return false;
}

// Set a configuration value as an integer
bool ConfigManager::setConfigInt(const std::string& group, const std::string& key, int value) {
    std::string strValue = std::to_string(value);
    if (!hasConfigValue(group, key) || configMap[group][key] != strValue) {
        configMap[group][key] = strValue;
        serializeConfig();
        return true;
    }
    return false;
}

// Serialize the configuration data using the fileManager
void ConfigManager::serializeConfig() {
    std::ostringstream dataStream;
    for (const auto& group : configMap) {
        dataStream << "[" << group.first << "]\n";
        for (const auto& pair : group.second) {
            dataStream << pair.first << "=" << pair.second << "\n";
        }
    }

    if (fileManager) {
        fileManager->writeData("ConfigManager", configFileName, dataStream.str());
        ESP_LOGI(TAG, "Configuration data serialized to file: %s", configFileName.c_str());
    } else {
        ESP_LOGE(TAG, "FileManager not initialized.");
    }
}

// Deserialize the configuration data using the fileManager
bool ConfigManager::deserializeConfig() {
    if (!fileManager) {
        ESP_LOGE(TAG, "FileManager not initialized.");
        return false;
    }

    std::string fileData = fileManager->readData("ConfigManager", configFileName);

    if (fileData.empty()) {
        ESP_LOGE(TAG, "Failed to read data from file: %s", configFileName.c_str());
        return false;
    }

    ESP_LOGI(TAG, "ConfigManager Data: %s", fileData.c_str());

    std::istringstream dataStream(fileData);
    std::string line;
    std::string currentGroup;
    bool success = false;
    bool groupHeaderFound = false;

    while (std::getline(dataStream, line)) {
        line = line.erase(0, line.find_first_not_of(" \t")); // Trim leading whitespace

        if (line.empty() || line[0] == '#') {
            continue; // skip empty lines and comments
        }

        if (line[0] == '[' && line.back() == ']') {
            currentGroup = line.substr(1, line.size() - 2);

            // Check for valid group name
            if (currentGroup.empty() || currentGroup.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-") != std::string::npos) {
                ESP_LOGE(TAG, "Invalid group name in configuration file: %s", line.c_str());
                return false;
            }

            groupHeaderFound = true;
        } else {
            size_t equalsPos = line.find('=');
            if (equalsPos != std::string::npos) {
                if (!groupHeaderFound) {
                    ESP_LOGE(TAG, "Key-value pair found before any group header: %s", line.c_str());
                    return false;
                }

                std::string key = line.substr(0, equalsPos);
                std::string value = line.substr(equalsPos + 1);

                // Validate key
                if (key.empty() || key.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-") != std::string::npos) {
                    ESP_LOGE(TAG, "Invalid key in configuration file: %s", line.c_str());
                    return false;
                }

                // Optionally validate value (e.g., no control characters)
                if (value.find_first_of("\n\r") != std::string::npos) {
                    ESP_LOGE(TAG, "Invalid value in configuration file: %s", line.c_str());
                    return false;
                }

                configMap[currentGroup][key] = value;
                success = true; // At least one key-value pair is successfully parsed
            } else {
                ESP_LOGE(TAG, "Invalid key-value pair in configuration file: %s", line.c_str());
                return false;
            }
        }
    }

    if (!groupHeaderFound) {
        ESP_LOGE(TAG, "No valid group header found in configuration file: %s", configFileName.c_str());
        return false;
    }

    if (success) {
        ESP_LOGI(TAG, "Configuration data successfully deserialized from file: %s", configFileName.c_str());
    } else {
        ESP_LOGW(TAG, "No valid configuration data found in file: %s", configFileName.c_str());
    }

    return success;
}