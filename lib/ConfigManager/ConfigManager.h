#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <map>
#include "nvs_handle.hpp"

class ConfigManager {
private:
    static std::map<std::string, std::map<std::string, std::string>> configMap;
    static std::map<std::string, std::map<std::string, std::string>> defaultConfigMap;
    static nvs_handle_t nvsHandle;

public:
    static void init();

    static std::string getConfigString(const std::string& group, const std::string& key);
    static int getConfigInt(const std::string& group, const std::string& key);
    static bool hasConfigValue(const std::string& group, const std::string& key);

    static bool setConfigString(const std::string& group, const std::string& key, const std::string& value);
    static bool setConfigInt(const std::string& group, const std::string& key, int value);

    static void serializeConfig();
    static bool deserializeConfig();

    static int64_t getConfig64(const std::string& group, const std::string& key);
    static bool setConfig64(const std::string& group, const std::string& key, int64_t value);
};


#endif // CONFIG_MANAGER_H