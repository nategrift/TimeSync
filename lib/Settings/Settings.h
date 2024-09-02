#ifndef SETTING_H
#define SETTING_H

#include "IApp.h"
#include "lvgl.h"
#include "AppManager.h"


// Enum for data types
enum class SettingType {
    INT,
    DOUBLE,
    DATE,
    TIME,
    BOOL,
    STRING,
    BUTTON,
};

// Callback types
using ReadCallback = std::function<std::string()>;
using DisplayValueCallback = std::function<std::string()>;
using WriteCallback = std::function<void(const std::string&)>;

// Structure for settings
struct Setting {
    std::string title;
    ReadCallback readCallback;
    WriteCallback writeCallback;
    DisplayValueCallback displayValueCallback;
    SettingType type;
    char* options;
};


class Settings : public IApp {
private:
    lv_obj_t* screenObj;
    AppManager& appManager;

public:
    Settings(AppManager& manager);
    ~Settings() override;

    void launch() override;
    void close() override;
    void backgroundActivity() override;

private:
};

#endif // SETTING_H
