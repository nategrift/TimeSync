#include "ConfigManager.h"
#include <sstream>
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "ConfigManager";

// Static member initialization
std::map<std::string, std::map<std::string, std::string>> ConfigManager::configMap;
std::map<std::string, std::map<std::string, std::string>> ConfigManager::defaultConfigMap = {
    {"General", {{"ScreenTimeout", "30"}, {"Time", "02:15:30"}, {"Date", "2024-08-01"}, {"Brightness", "10"}, {"Volume", "5"}, {"Mute", "1"}, {"Name", "TimeSync"}}},
    {"Network", {{"Enabled", "0"}, {"SSID", ""}, {"Password", ""}}}
};
nvs_handle_t ConfigManager::nvsHandle;

// Initialize the ConfigManager with NVS
void ConfigManager::init() {
    esp_err_t err = nvs_open("config", NVS_READWRITE, &nvsHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
    }
    
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

// Serialize the configuration data using NVS
void ConfigManager::serializeConfig() {
    for (const auto& group : configMap) {
        for (const auto& pair : group.second) {
            std::string key = group.first.substr(0, 2) + "_" + pair.first.substr(0, 12);
            esp_err_t err = nvs_set_str(nvsHandle, key.c_str(), pair.second.c_str());
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error saving key %s: %s", key.c_str(), esp_err_to_name(err));
            }
        }
    }
    
    esp_err_t err = nvs_commit(nvsHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS changes: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Configuration data serialized to NVS");
    }
}

// Deserialize the configuration data using NVS
bool ConfigManager::deserializeConfig() {
    configMap.clear();
    bool success = false;

    // Iterate through default config to load stored values
    for (const auto& group : defaultConfigMap) {
        for (const auto& pair : group.second) {
            std::string key = group.first.substr(0, 2) + "_" + pair.first.substr(0, 12);
            size_t required_size;
            
            // First get required size
            esp_err_t err = nvs_get_str(nvsHandle, key.c_str(), nullptr, &required_size);
            if (err == ESP_OK) {
                char* value = new char[required_size];
                err = nvs_get_str(nvsHandle, key.c_str(), value, &required_size);
                if (err == ESP_OK) {
                    configMap[group.first][pair.first] = std::string(value);
                    success = true;
                }
                delete[] value;
            } else if (err == ESP_ERR_NVS_NOT_FOUND) {
                // Use default value if not found in NVS
                configMap[group.first][pair.first] = pair.second;
                success = true;
            } else {
                ESP_LOGE(TAG, "Error reading key %s: %s", key.c_str(), esp_err_to_name(err));
            }
        }
    }

    if (success) {
        ESP_LOGI(TAG, "Configuration data successfully deserialized from NVS");
    } else {
        ESP_LOGW(TAG, "No valid configuration data found in NVS");
    }

    return success;
}