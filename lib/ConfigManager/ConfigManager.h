#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <map>
#include "FileManager.h"

class ConfigManager {
private:
    static std::map<std::string, std::map<std::string, std::string>> configMap;
    static std::map<std::string, std::map<std::string, std::string>> defaultConfigMap;
    static std::string configFileName;
    static FileManager* fileManager;

public:
    static void init(FileManager& fm, const std::string& filename);

    static std::string getConfigString(const std::string& group, const std::string& key);
    static int getConfigInt(const std::string& group, const std::string& key);
    static bool hasConfigValue(const std::string& group, const std::string& key);

    static bool setConfigString(const std::string& group, const std::string& key, const std::string& value);
    static bool setConfigInt(const std::string& group, const std::string& key, int value);

    static void serializeConfig();
    static bool deserializeConfig();
};


#endif // CONFIG_MANAGER_H