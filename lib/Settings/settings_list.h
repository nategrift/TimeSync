
#ifndef SETTINGS_LIST_H
#define SETTINGS_LIST_H

#include "lvgl.h"
#include "Settings.h"
#include <vector>
#include <string>
#include "settings_edit.h"
#include "esp_log.h"

// Function to create a settings list for a category using a flex container
lv_obj_t* create_settings_flex(lv_obj_t* parent, std::vector<Setting>& settings, AppManager& appManager) {
    // Create a container for the settings


    //add padding at top and bottom to parent
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_size(container, lv_pct(90), lv_pct(100));
    lv_obj_center(container);

    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);

    lv_obj_set_style_pad_ver(container, LV_HOR_RES/2, LV_PART_MAIN);


     // Create the back button
    lv_obj_t* backBtn = lv_btn_create(container);
    lv_obj_set_width(backBtn, lv_pct(100)); // Set initial width
    lv_obj_set_style_pad_all(backBtn, 15, LV_PART_MAIN); 
    lv_obj_set_style_bg_color(backBtn, lv_color_hex(0x083445), LV_PART_MAIN);
    lv_obj_center(backBtn);

    // Create a label for the button
    lv_obj_t* label = lv_label_create(backBtn);
    lv_label_set_text(label, "Back to Home");
    lv_obj_set_style_text_color(label, lv_color_hex(0x7ab7cf), LV_PART_MAIN);
    lv_obj_set_align(label, LV_ALIGN_CENTER);

    // go back to app selector. Back button
    lv_obj_add_event_cb(backBtn, [](lv_event_t* e) {
        AppManager* appManager = (AppManager*)(lv_event_get_user_data(e));
        if (appManager) {
            appManager->launchApp("AppSelector");
            
        }
    }, LV_EVENT_CLICKED, &appManager);

    for (size_t i = 0; i < settings.size(); ++i) {
        Setting& setting = settings[i];

        // Create a button for each setting
        lv_obj_t* btn = lv_btn_create(container);
        lv_obj_set_width(btn, lv_pct(100)); // Set initial width
        lv_obj_set_style_pad_all(btn, 15, LV_PART_MAIN); // Add padding to the button
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x111111), LV_PART_MAIN); // Dark grey button color
        lv_obj_center(btn);

        // Create a label for the button
        
        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, (setting.title + ":").c_str());
        lv_obj_set_style_text_color(label, lv_color_hex(0xDDDDDD), LV_PART_MAIN); // Almost white text
        lv_obj_set_align(label, LV_ALIGN_LEFT_MID);

        lv_obj_t* value = lv_label_create(btn);
        lv_label_set_text(value, setting.readCallback().c_str());
        lv_obj_set_style_text_color(value, lv_color_hex(0xDDDDDD), LV_PART_MAIN); // Almost white text
        lv_obj_set_align(value, LV_ALIGN_RIGHT_MID);


        // Add an event callback to open the edit screen
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            lv_event_code_t code = lv_event_get_code(e);
            if (code == LV_EVENT_CLICKED) {
                Setting* setting = (Setting*)(lv_event_get_user_data(e));
                if (setting) {
                    open_edit_screen(setting);
                    
                }
            }
        }, LV_EVENT_CLICKED, &setting);
    }

    // // Event handler for scrolling to adjust button sizes
    // lv_obj_add_event_cb(container, [](lv_event_t* e) {
    //     lv_event_code_t code = lv_event_get_code(e);
    //     if (code == LV_EVENT_SCROLL) {
    //         lv_obj_t* cont = lv_event_get_target(e);
    //         lv_coord_t cont_h = lv_obj_get_height(cont);
    //         lv_coord_t y_offset = lv_obj_get_scroll_y(cont);

    //         // Adjust the scale of each button based on its position
    //         // uint32_t child_count = lv_obj_get_child_cnt(cont);
    //         // for (uint32_t i = 0; i < child_count; i++) {
    //         //     lv_obj_t* child = lv_obj_get_child(cont, i);
    //         //     lv_coord_t child_y = lv_obj_get_y(child) + y_offset;
    //         //     lv_coord_t child_h = lv_obj_get_height(child);
    //         //     lv_coord_t center_offset = abs((cont_h / 2) - (child_y + child_h / 2));
    //         //     float scale = 1.0f - (center_offset / (float)cont_h);

    //         //     if (scale < 0.5f) scale = 0.5f; // Minimum scale
    //         //     lv_obj_set_style_transform_width(child, scale * 100, LV_PART_MAIN);
    //         //     // lv_obj_set_style_transform_height(child, scale * 100, LV_PART_MAIN);
    //         // }
    //     }
    // }, LV_EVENT_SCROLL, NULL);

    return container;
}

#endif // SETTINGS_LIST_H
