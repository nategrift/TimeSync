#include "esp_spiffs.h"
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include "esp_log.h"

#define BASE_PATH "/spiffs"
#define MAX_FILES 5

void file_manager_init() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = BASE_PATH,
        .partition_label = NULL,
        .max_files = MAX_FILES,
        .format_if_mount_failed = true
    };

    esp_vfs_spiffs_unregister(NULL);

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

void file_manager_deinit() {
    esp_vfs_spiffs_unregister(NULL);
}

int file_manager_write_data(const char* app, const char* filename, const char* data) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s/%s", BASE_PATH, app, filename);
    FILE* file = fopen(filepath, "w");
    if (!file) {
        ESP_LOGE("FileManager", "Failed to open file for writing: %s", filepath);
        return 0;
    }
    fprintf(file, "%s", data);
    fclose(file);
    return 1;
}

int file_manager_append_data(const char* app, const char* filename, const char* data) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s/%s", BASE_PATH, app, filename);
    FILE* file = fopen(filepath, "a");
    if (!file) {
        ESP_LOGE("FileManager", "Failed to open file for appending: %s", filepath);
        return 0;
    }
    fprintf(file, "%s", data);
    fclose(file);
    return 1;
}

char* file_manager_read_data_root(const char* path) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", BASE_PATH, path);
    FILE* file = fopen(filepath, "r");
    if (!file) {
        ESP_LOGE("FileManager", "Failed to open file for reading: %s", filepath);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* data = (char*)malloc(length + 1);
    if (data) {
        fread(data, 1, length, file);
        data[length] = '\0';
    }
    fclose(file);
    return data;
}

char* file_manager_read_data(const char* app, const char* filename) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", app, filename);
    return file_manager_read_data_root(filepath);
}


void file_manager_get_files_in_directory(const char* app, char*** files, int* count) {
    char dirPath[256];
    snprintf(dirPath, sizeof(dirPath), "%s/%s", BASE_PATH, app);
    DIR* dir = opendir(dirPath);
    if (dir == NULL) {
        ESP_LOGE("FileManager", "Failed to open directory: %s", dirPath);
        *files = NULL;
        *count = 0;
        return;
    }

    struct dirent* entry;
    *count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            (*count)++;
        }
    }
    rewinddir(dir);

    *files = (char**)malloc(*count * sizeof(char*));
    int i = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            (*files)[i] = strdup(entry->d_name);
            i++;
        }
    }
    closedir(dir);
}