#include "FileManager.h"
#include "esp_spiffs.h"
#include <fstream>
#include <sys/stat.h>
#include <iostream>
#include "esp_log.h"
#include <cerrno>
#include <cstring>
#include <string>

FileManager::FileManager() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE("FileManager", "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE("FileManager", "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE("FileManager", "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE("FileManager", "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI("FileManager", "Partition size: total: %d, used: %d", total, used);
    }
}

FileManager::~FileManager() {
    esp_vfs_spiffs_unregister(NULL);
}


bool FileManager::writeData(const std::string& app, const std::string& filename, const std::string& data) {
    std::string dirPath = "/spiffs/" + app;
    std::string filepath = dirPath + "/" + filename;
    std::ofstream file(filepath);
    if (!file) {
        ESP_LOGE("FileManager", "Failed to open file for writing: %s", filepath.c_str());
        return false;
    }
    file << data;
    file.close();
    return true;
}

std::string FileManager::readData(const std::string& app, const std::string& filename) {
    std::string dirPath = "/spiffs/" + app;
    std::string filepath = dirPath + "/" + filename;
    std::ifstream file(filepath);
    if (!file) {
        ESP_LOGE("FileManager", "Failed to open file for reading: %s", filepath.c_str());
        return "";
    }
    std::string data;
    std::string line;
    while (std::getline(file, line)) {
        data += line + "\n";
    }
    if (!data.empty() && data.back() == '\n') {
        data.pop_back();  // Remove the last newline if it exists
    }
    file.close();
    return data;
}
