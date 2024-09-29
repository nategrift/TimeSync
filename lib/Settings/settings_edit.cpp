#include "settings_edit.h"
#include <string>
#include <cstdio>


lv_obj_t* edit_screen = nullptr;
lv_obj_t* previous_screen  = nullptr;

std::string create_roller_options(int start, int end) {
    std::string options;
    for (int i = start; i <= end; ++i) {
        options += std::to_string(i);
        if (i < end) options += "\n";
    }
    return options;
}


lv_obj_t* get_settings_edit_screen(Setting* setting) {
    lv_obj_t * screen = get_blank_screen(NULL);

    lv_obj_t* label = lv_label_create(screen);
    lv_label_set_text(label, setting->title.c_str());
    lv_obj_set_style_text_color(label, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 14);

    
    lv_obj_t * saveButton = get_button(screen, "Back");
    lv_obj_align(saveButton, LV_ALIGN_BOTTOM_MID, 0, -14);

    lv_obj_add_event_cb(saveButton, [](lv_event_t * e) {
        lv_obj_t * btn = lv_event_get_target(e);
        lv_obj_t * background = lv_obj_get_parent(btn);
        
        ESP_LOGI("SETTINGS_EDIT_SCREEN", "Going back");
        lv_scr_load_anim(previous_screen, LV_SCR_LOAD_ANIM_OVER_RIGHT, 500, 0, true);
    }, LV_EVENT_CLICKED, nullptr);

    return screen;
}


lv_obj_t* create_int_edit_screen(Setting* setting) {
    lv_obj_t* screen = get_settings_edit_screen(setting);

    
    lv_obj_t * roller = get_default_roller(screen);
    lv_obj_center(roller);
    lv_roller_set_options(roller, setting->options, LV_ROLLER_MODE_INFINITE);
    const char* currentSelected = setting->readCallback().c_str();
    int selectedIndex = find_roller_selected_index(setting->options, currentSelected);
    lv_roller_set_selected(roller, selectedIndex, LV_ANIM_ON);

    lv_obj_add_event_cb(roller, [](lv_event_t* e) {
        lv_obj_t* roller = lv_event_get_target(e);
        Setting* setting = (Setting*)(lv_event_get_user_data(e));
        char value[10];
        lv_roller_get_selected_str(roller, value, sizeof(value));
        std::string cpp_string(value);
        setting->writeCallback(cpp_string);
    }, LV_EVENT_VALUE_CHANGED, setting);
    return screen;
}



lv_obj_t* create_time_edit_screen(Setting* setting) {
    lv_obj_t* screen = get_settings_edit_screen(setting);

    lv_obj_t* label = lv_label_create(screen);
    lv_label_set_text(label, "HH-MM-SS");
    lv_obj_set_style_text_color(label, COLOR_MUTED_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t *hourRoller = get_default_roller(screen);
    lv_obj_t *minuteRoller = get_default_roller(screen);
    lv_obj_t *secondRoller = get_default_roller(screen);
    lv_obj_align(hourRoller, LV_ALIGN_CENTER, -60, 0);
    lv_obj_align(minuteRoller, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(secondRoller, LV_ALIGN_CENTER, 60, 0);

    // Options for the rollers
    std::string hourOptions = create_roller_options(0, 23);
    std::string minuteOptions = create_roller_options(0, 59);
    std::string secondOptions = create_roller_options(0, 59);

    int hour, minute, second = 0;
    sscanf(setting->readCallback().c_str(), "%d:%d:%d", &hour, &minute, &second);

    lv_roller_set_options(hourRoller, hourOptions.c_str(), LV_ROLLER_MODE_NORMAL);
    lv_roller_set_options(minuteRoller, minuteOptions.c_str(), LV_ROLLER_MODE_NORMAL);
    lv_roller_set_options(secondRoller, secondOptions.c_str(), LV_ROLLER_MODE_NORMAL);

    lv_roller_set_selected(hourRoller, hour, LV_ANIM_OFF);
    lv_roller_set_selected(minuteRoller, minute, LV_ANIM_OFF);
    lv_roller_set_selected(secondRoller, second, LV_ANIM_OFF);

    // Event callback for each roller to save selected time
    auto event_cb = [](lv_event_t* e) {
        lv_obj_t* currentObj = lv_event_get_target(e);

        lv_obj_t* hourRoller = lv_obj_get_child(lv_obj_get_parent(currentObj), -3);
        lv_obj_t* minuteRoller = lv_obj_get_child(lv_obj_get_parent(currentObj), -2);
        lv_obj_t* secondRoller = lv_obj_get_child(lv_obj_get_parent(currentObj), -1);
        Setting* setting = (Setting*)(lv_event_get_user_data(e));
        if (hourRoller == nullptr || minuteRoller == nullptr || secondRoller == nullptr) {
            ESP_LOGI("TIME_EDIT_SCREEN", "Hour, Minute, or Second roller is null");
            return;
        }

        char hourStr[3], minuteStr[3], secondStr[3];
        lv_roller_get_selected_str(hourRoller, hourStr, sizeof(hourStr));
        lv_roller_get_selected_str(minuteRoller, minuteStr, sizeof(minuteStr));
        lv_roller_get_selected_str(secondRoller, secondStr, sizeof(secondStr));

        std::string formattedTime = std::string(hourStr) + ":" + std::string(minuteStr) + ":" + std::string(secondStr);
        setting->writeCallback(formattedTime);
    };

    lv_obj_add_event_cb(hourRoller, event_cb, LV_EVENT_VALUE_CHANGED, setting);
    lv_obj_add_event_cb(minuteRoller, event_cb, LV_EVENT_VALUE_CHANGED, setting);
    lv_obj_add_event_cb(secondRoller, event_cb, LV_EVENT_VALUE_CHANGED, setting);

    return screen;
}




lv_obj_t* create_date_edit_screen(Setting* setting) {
    lv_obj_t* screen = get_settings_edit_screen(setting);

    lv_obj_t* label = lv_label_create(screen);
    lv_label_set_text(label, "DD-MM-YYYY");
    lv_obj_set_style_text_color(label, COLOR_MUTED_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t *dayRoller = get_default_roller(screen);
    lv_obj_t *monthRoller = get_default_roller(screen);
    lv_obj_t *yearRoller = get_default_roller(screen);
    lv_obj_align(dayRoller, LV_ALIGN_CENTER, -65, 0);
    lv_obj_align(monthRoller, LV_ALIGN_CENTER, -5, 0);
    lv_obj_align(yearRoller, LV_ALIGN_CENTER, 65, 0);

    // Options for the rollers
    std::string dayOptions = create_roller_options(1, 30);
    std::string monthOptions = create_roller_options(1, 12);
    std::string yearOptions = create_roller_options(2020, 2040);

    int day, month, year = 0;
    sscanf(setting->readCallback().c_str(), "%d-%d-%d", &year, &month, &day);

    lv_roller_set_options(dayRoller, dayOptions.c_str(), LV_ROLLER_MODE_NORMAL);
    lv_roller_set_options(monthRoller, monthOptions.c_str(), LV_ROLLER_MODE_NORMAL);
    lv_roller_set_options(yearRoller, yearOptions.c_str(), LV_ROLLER_MODE_NORMAL);

    lv_roller_set_selected(dayRoller, day-1, LV_ANIM_OFF);
    lv_roller_set_selected(monthRoller, month-1, LV_ANIM_OFF);
    const char* currentSelectedYear = setting->readCallback().c_str();
    int selectedIndexYear = find_roller_selected_index(const_cast<char*>(yearOptions.c_str()), const_cast<char*>(std::to_string(year).c_str()));
    lv_roller_set_selected(yearRoller, selectedIndexYear, LV_ANIM_ON);

    // Event callback for each roller to save selected time
    auto event_cb = [](lv_event_t* e) {
        lv_obj_t* currentObj = lv_event_get_target(e);

        lv_obj_t* dayRoller = lv_obj_get_child(lv_obj_get_parent(currentObj), -3);
        lv_obj_t* monthRoller = lv_obj_get_child(lv_obj_get_parent(currentObj), -2);
        lv_obj_t* yearRoller = lv_obj_get_child(lv_obj_get_parent(currentObj), -1);
        Setting* setting = (Setting*)(lv_event_get_user_data(e));
        if (dayRoller == nullptr || monthRoller == nullptr || yearRoller == nullptr) {
            ESP_LOGI("TIME_EDIT_SCREEN", "Day, Month, or Year roller is null");
            return;
        }

        char dayStr[5], monthStr[5], yearStr[5];
        lv_roller_get_selected_str(dayRoller, dayStr, sizeof(dayStr));
        lv_roller_get_selected_str(monthRoller, monthStr, sizeof(monthStr));
        lv_roller_get_selected_str(yearRoller, yearStr, sizeof(yearStr));

        std::string formattedTime = std::string(yearStr) + "-" + std::string(monthStr) + "-" + std::string(dayStr);
        setting->writeCallback(formattedTime);
    };

    lv_obj_add_event_cb(dayRoller, event_cb, LV_EVENT_VALUE_CHANGED, setting);
    lv_obj_add_event_cb(monthRoller, event_cb, LV_EVENT_VALUE_CHANGED, setting);
    lv_obj_add_event_cb(yearRoller, event_cb, LV_EVENT_VALUE_CHANGED, setting);

    return screen;
}

lv_obj_t* create_button_edit_screen(Setting* setting) {
    lv_obj_t* screen = get_settings_edit_screen(setting);

    lv_obj_t * actionButton = get_button(screen, const_cast<char*>(setting->title.c_str()));
    lv_obj_center(actionButton);

    lv_obj_add_event_cb(actionButton, [](lv_event_t* e) {
        Setting* setting = (Setting*)(lv_event_get_user_data(e));
        setting->writeCallback("");
    }, LV_EVENT_CLICKED, setting);

    return screen;
}

lv_obj_t* create_bool_edit_screen(Setting* setting) {
    lv_obj_t* screen = get_settings_edit_screen(setting);

    lv_obj_t * sw = lv_switch_create(screen);
    lv_obj_center(sw);

    // Set the initial state of the switch
    bool currentState = (setting->readCallback() == "1");
    lv_obj_add_state(sw, currentState ? LV_STATE_CHECKED : 0);

    lv_obj_add_event_cb(sw, [](lv_event_t* e) {
        lv_obj_t* sw = lv_event_get_target(e);
        Setting* setting = (Setting*)(lv_event_get_user_data(e));
        bool newState = lv_obj_has_state(sw, LV_STATE_CHECKED);
        setting->writeCallback(newState ? "1" : "0");
    }, LV_EVENT_VALUE_CHANGED, setting);

    return screen;
}

// Function to open an edit screen
void open_edit_screen(Setting* setting) {
    previous_screen = lv_scr_act();
    ESP_LOGI("SETTINGS_LISt", "%s", setting->title.c_str());
    switch (setting->type) {
        case SettingType::INT:
            edit_screen = create_int_edit_screen(setting);
            break;
        case SettingType::DOUBLE:
            // create_double_edit_screen(screen, setting);
            break;
        case SettingType::DATE:
            edit_screen = create_date_edit_screen(setting);
            break;
        case SettingType::TIME:
            edit_screen = create_time_edit_screen(setting);
            break;
        case SettingType::BOOL:
            edit_screen = create_bool_edit_screen(setting);
            break;
        case SettingType::STRING:
            // create_string_edit_screen(screen, setting);
            break;
        case SettingType::BUTTON:
            edit_screen = create_button_edit_screen(setting);
            break;
    }
    lv_scr_load_anim(edit_screen, LV_SCR_LOAD_ANIM_OVER_LEFT, 500, 0, false);
}
