#ifndef SETTINGS_EDIT_UI_H
#define SETTINGS_EDIT_UI_H

#include "lvgl.h"
#include "Settings.h"
#include "esp_log.h"
#include "app_screen.h"
#include "ui_components.h"

lv_obj_t* edit_screen = nullptr;
lv_obj_t* previous_screen  = nullptr;

lv_obj_t* get_settings_edit_screen(Setting* setting) {
    lv_obj_t * screen = get_blank_screen();

    lv_obj_t* label = lv_label_create(screen);
    lv_label_set_text(label, setting->title.c_str());
    lv_obj_set_style_text_color(label, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 14);

    lv_obj_t* saveButton = lv_btn_create(screen);
    lv_obj_align(saveButton, LV_ALIGN_BOTTOM_MID, 0, -14);
    lv_obj_set_height(saveButton, LV_SIZE_CONTENT);

    lv_obj_t* saveButtonLabel = lv_label_create(saveButton);
    lv_label_set_text(saveButtonLabel, "Back");
    lv_obj_center(saveButtonLabel);

    lv_obj_add_event_cb(saveButton, [](lv_event_t * e) {
        lv_obj_t * btn = lv_event_get_target(e);
        lv_obj_t * background = lv_obj_get_parent(btn);

        // Delete the background object, which deletes all child nodes
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
            // create_date_edit_screen(screen, setting);
            break;
        case SettingType::TIME:
            // create_time_edit_screen(screen, setting);
            break;
        case SettingType::BOOL:
            // create_bool_edit_screen(screen, setting);
            break;
        case SettingType::STRING:
            // create_string_edit_screen(screen, setting);
            break;
    }
    lv_scr_load_anim(edit_screen, LV_SCR_LOAD_ANIM_OVER_LEFT, 500, 0, false);
}

#endif // SETTINGS_EDIT_UI_H