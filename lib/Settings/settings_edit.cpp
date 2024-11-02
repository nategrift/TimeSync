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


lv_obj_t* get_settings_edit_screen(Setting* setting, lv_obj_t* previous_scr) {
    lv_obj_t * screen = get_blank_screen(NULL);

    lv_obj_t* label = lv_label_create(screen);
    lv_label_set_text(label, setting->title.c_str());
    lv_obj_set_style_text_color(label, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 14);

    char* buttonLabel = "Back";
    lv_obj_t * saveButton = get_button(screen, buttonLabel);
    lv_obj_align(saveButton, LV_ALIGN_BOTTOM_MID, 0, -14);

    lv_obj_add_event_cb(saveButton, [](lv_event_t * e) {
        lv_obj_t * btn = lv_event_get_target(e);
        
        ESP_LOGI("SETTINGS_EDIT_SCREEN", "Going back");
        lv_obj_t* previous_scr = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
        if (previous_scr) {
            lv_scr_load_anim(previous_scr, LV_SCR_LOAD_ANIM_OVER_RIGHT, 500, 0, true);
        }
    }, LV_EVENT_CLICKED, previous_scr);

    return screen;
}


lv_obj_t* create_int_edit_screen(Setting* setting, lv_obj_t* previous_scr) {
    lv_obj_t* screen = get_settings_edit_screen(setting, previous_scr);

    
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



lv_obj_t* create_time_edit_screen(Setting* setting, lv_obj_t* previous_scr) {
    lv_obj_t* screen = get_settings_edit_screen(setting, previous_scr);

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

lv_obj_t* create_date_edit_screen(Setting* setting, lv_obj_t* previous_scr) {
    lv_obj_t* screen = get_settings_edit_screen(setting, previous_scr);

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

lv_obj_t* create_button_edit_screen(Setting* setting, lv_obj_t* previous_scr) {
    lv_obj_t* screen = get_settings_edit_screen(setting, previous_scr);

    lv_obj_t * actionButton = get_button(screen, const_cast<char*>(setting->title.c_str()));
    lv_obj_center(actionButton);

    lv_obj_add_event_cb(actionButton, [](lv_event_t* e) {
        Setting* setting = (Setting*)(lv_event_get_user_data(e));
        setting->writeCallback("");
    }, LV_EVENT_CLICKED, setting);

    return screen;
}

lv_obj_t* create_bool_edit_screen(Setting* setting, lv_obj_t* previous_scr) {
    lv_obj_t* screen = get_settings_edit_screen(setting, previous_scr);

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


static const char* btnm_map_ABC[] = {"ABC", "DEF", "GHI", LV_SYMBOL_BACKSPACE, "\n",
                                     "JKL", "MNO", "PQRS", LV_SYMBOL_UP, "\n",
                                     "TUV", "WXYZ", ".,?!", LV_SYMBOL_OK, "\n",
                                     " ", "0", "Space", " ", ""};

static const char* btnm_map_abc[] = {"abc", "def", "ghi", LV_SYMBOL_BACKSPACE, "\n",
                                     "jkl", "mno", "pqrs", LV_SYMBOL_UP, "\n",
                                     "tuv", "wxyz", ".,?!", LV_SYMBOL_OK, "\n",
                                     " ", "0", "Space", " ", ""};

static const char* btnm_map_123[] = {"1", "2", "3", LV_SYMBOL_BACKSPACE, "\n",
                                     "4", "5", "6", LV_SYMBOL_UP, "\n",
                                     "7", "8", "9", LV_SYMBOL_OK, "\n",
                                     " ", "0", "Space", " ", ""};

static const char* btnm_map_symbols[] = {"`@#", "$%^", "&*-", LV_SYMBOL_BACKSPACE, "\n",
                                         "({[", "]})", "_+=", LV_SYMBOL_UP, "\n",
                                         "|\\/", ";:'", "\"<>", LV_SYMBOL_OK, "\n",
                                         " ", ",.?!~", "Space", " ", ""};

static const char** current_map = btnm_map_ABC;
static int map_index = 0;
static const int NUM_MAPS = 4;
static const char*** all_maps = new const char**[NUM_MAPS]{btnm_map_ABC, btnm_map_abc, btnm_map_123, btnm_map_symbols};

lv_obj_t* create_string_edit_screen(Setting* setting, lv_obj_t* previous_scr) {
    lv_obj_t* screen = get_settings_edit_screen(setting, previous_scr);

    // Create a text area
    lv_obj_t* text_area = lv_textarea_create(screen);
    lv_obj_set_size(text_area, 170, 36);
    lv_obj_align(text_area, LV_ALIGN_TOP_MID, 0, 45);
    lv_textarea_set_text(text_area, setting->readCallback().c_str());
    lv_obj_set_style_text_color(text_area, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_set_style_bg_color(text_area, COLOR_INPUT_BACKGROUND, LV_PART_MAIN);
    lv_obj_add_state(text_area, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(text_area, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_shadow_width(text_area, 0, LV_PART_MAIN | LV_STATE_FOCUSED);

    lv_obj_t* btnm = lv_btnmatrix_create(screen);
    lv_btnmatrix_set_map(btnm, current_map);
    lv_obj_align(btnm, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_width(btnm, 220);
    lv_obj_set_height(btnm, 150);
    lv_obj_set_style_bg_color(btnm, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btnm, COLOR_INPUT_BACKGROUND, LV_PART_ITEMS);
    lv_obj_set_style_shadow_width(btnm, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btnm, 0, LV_PART_ITEMS);
    lv_obj_set_style_border_width(btnm, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(btnm, 0, LV_PART_ITEMS);
    lv_obj_set_style_text_color(btnm, COLOR_TEXT, LV_PART_MAIN | LV_PART_ITEMS);
    lv_obj_set_style_pad_all(btnm, 0, LV_PART_ITEMS);
    lv_obj_set_style_pad_gap(btnm, 0, LV_PART_ITEMS);

    lv_btnmatrix_set_btn_ctrl(btnm, 12, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_btnmatrix_set_btn_ctrl(btnm, 15, LV_BTNMATRIX_CTRL_HIDDEN);

    static int last_btn = -1;
    static uint32_t last_click_time = 0;
    static int click_count = 0;

    lv_obj_add_event_cb(btnm, [](lv_event_t* e) {
        lv_obj_t* obj = lv_event_get_target(e);
        lv_obj_t* ta = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
        Setting* setting = static_cast<Setting*>(lv_obj_get_user_data(obj));
        lv_obj_t* previous_scr = static_cast<lv_obj_t*>(lv_obj_get_user_data(ta));
        uint32_t id = lv_btnmatrix_get_selected_btn(obj);
        const char* txt = lv_btnmatrix_get_btn_text(obj, id);

        if (txt == NULL) return;

        if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
            lv_textarea_del_char(ta);
        } else if (strcmp(txt, LV_SYMBOL_OK) == 0) {
            // Handle OK button: save the current text and exit
            const char* current_text = lv_textarea_get_text(ta);
            setting->writeCallback(current_text);

            if (previous_scr) {
                lv_scr_load_anim(previous_scr, LV_SCR_LOAD_ANIM_OVER_RIGHT, 500, 0, true);
            }
        } else if (strcmp(txt, LV_SYMBOL_UP) == 0) {
            map_index = (map_index + 1) % NUM_MAPS;
            current_map = all_maps[map_index];
            lv_btnmatrix_set_map(obj, current_map);
        } else if (strcmp(txt, "Space") == 0) {
            lv_textarea_add_char(ta, ' ');
        } else {
            uint32_t current_time = lv_tick_get();
            if (id == last_btn && current_time - last_click_time < 1000) {
                click_count++;
                lv_textarea_del_char(ta);
            } else {
                click_count = 0;
            }

            int rowOffset = id / 4;
            const char* btn_text = current_map[rowOffset + id];
            char c = btn_text[click_count % strlen(btn_text)];
            lv_textarea_add_char(ta, c);

            last_btn = id;
            last_click_time = current_time;
        }
    }, LV_EVENT_VALUE_CHANGED, text_area);

    // Set user data for the button matrix and text area
    lv_obj_set_user_data(btnm, setting);
    lv_obj_set_user_data(text_area, previous_scr);

    return screen;
}

// Function to open an edit screen
void open_edit_screen(Setting* setting) {
    previous_screen = lv_scr_act();
    ESP_LOGI("SETTINGS_LIST", "%s", setting->title.c_str());
    switch (setting->type) {
        case SettingType::INT:
            edit_screen = create_int_edit_screen(setting, previous_screen);
            break;
        case SettingType::DOUBLE:
            // create_double_edit_screen(screen, setting);
            break;
        case SettingType::DATE:
            edit_screen = create_date_edit_screen(setting, previous_screen);
            break;
        case SettingType::TIME:
            edit_screen = create_time_edit_screen(setting, previous_screen);
            break;
        case SettingType::BOOL:
            edit_screen = create_bool_edit_screen(setting, previous_screen);
            break;
        case SettingType::STRING:
            edit_screen = create_string_edit_screen(setting, previous_screen);
            break;
        case SettingType::BUTTON:
            edit_screen = create_button_edit_screen(setting, previous_screen);
            break;
    }
    lv_scr_load_anim(edit_screen, LV_SCR_LOAD_ANIM_OVER_LEFT, 500, 0, false);
}