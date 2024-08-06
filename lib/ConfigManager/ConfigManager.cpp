#include "ConfigManager.h"
#include <sstream>
#include "esp_log.h"

static const char *TAG = "ConfigManager";

// Static member initialization
std::map<std::string, std::map<std::string, std::string>> ConfigManager::configMap;
std::map<std::string, std::map<std::string, std::string>> ConfigManager::defaultConfigMap = {
    {"General", {{"ScreenTimeout", "30"}}}
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
        ESP_LOGW(TAG, "Using default config value for group: %s, key: %s", group.c_str(), key.c_str());
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
        ESP_LOGW(TAG, "Using default config value for group: %s, key: %s", group.c_str(), key.c_str());
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
    configMap[group][key] = value;
    serializeConfig();
    return true;
}

// Set a configuration value as an integer
bool ConfigManager::setConfigInt(const std::string& group, const std::string& key, int value) {
    configMap[group][key] = std::to_string(value);
    serializeConfig();
    return true;
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

    std::istringstream dataStream(fileData);
    std::string line;
    std::string currentGroup;
    bool success = false;

    while (std::getline(dataStream, line)) {
        if (line.empty() || line[0] == '#') {
            continue; // skip empty lines and comments
        }

        if (line[0] == '[' && line.back() == ']') {
            currentGroup = line.substr(1, line.size() - 2);
        } else {
            size_t equalsPos = line.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = line.substr(0, equalsPos);
                std::string value = line.substr(equalsPos + 1);
                configMap[currentGroup][key] = value;
                success = true; // At least one key-value pair is successfully parsed
            }
        }
    }

    if (success) {
        ESP_LOGI(TAG, "Configuration data successfully deserialized from file: %s", configFileName.c_str());
    } else {
        ESP_LOGW(TAG, "No valid configuration data found in file: %s", configFileName.c_str());
    }

    return success;
}
