#ifndef SETTINGS_EDIT_UI_H
#define SETTINGS_EDIT_UI_H

#include "lvgl.h"
#include "Settings.h"
#include "esp_log.h"

void create_int_edit_screen(lv_obj_t* parent, Setting* setting) {
    lv_obj_t* slider = lv_slider_create(parent);
    lv_obj_set_width(slider, lv_pct(80));

    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, std::stoi(setting->readCallback()), LV_ANIM_OFF);
    lv_obj_center(slider);

    lv_obj_add_event_cb(slider, [](lv_event_t* e) {
        lv_obj_t* slider = lv_event_get_target(e);
        Setting* setting = (Setting*)(lv_event_get_user_data(e));
        int value = lv_slider_get_value(slider);
        setting->writeCallback(std::to_string(value));
    }, LV_EVENT_VALUE_CHANGED, setting);
    ESP_LOGI("Created", "CHEREt");
}


// Function to open an edit screen
void open_edit_screen(Setting* setting) {
    lv_obj_t* container = lv_obj_create(lv_scr_act());
    lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_size(container, LV_HOR_RES, LV_VER_RES);
    lv_obj_center(container);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x000000), LV_PART_MAIN);
    
    
    lv_obj_t* label = lv_label_create(container);
    std::string labelText = "Edit " + setting->title;
    lv_label_set_text(label, labelText.c_str());
    lv_obj_set_style_text_color(label, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t* saveButton = lv_btn_create(container);
    lv_obj_align(saveButton, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_height(saveButton, LV_SIZE_CONTENT);

    lv_obj_t* saveButtonLabel = lv_label_create(saveButton);
    lv_label_set_text(saveButtonLabel, "Save");
    lv_obj_center(saveButtonLabel);

    lv_obj_add_event_cb(saveButton, [](lv_event_t * e) {
        lv_obj_t * btn = lv_event_get_target(e);
        lv_obj_t * background = lv_obj_get_parent(btn);

        // Delete the background object, which deletes all child nodes
        lv_obj_del(background);
    }, LV_EVENT_CLICKED, nullptr);

    ESP_LOGI("SETTINGS_LISt", "%s", setting->title.c_str());
    switch (setting->type) {
        case SettingType::INT:
            create_int_edit_screen(container, setting);
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
}

#endif // SETTINGS_EDIT_UI_H