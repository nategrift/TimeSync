#include "ui_components.h"

lv_obj_t* get_default_roller(lv_obj_t* parent) {
    lv_obj_t* roller = lv_roller_create(parent);

    // main
    lv_obj_set_style_bg_color(roller, COLOR_INPUT_BACKGROUND, LV_PART_MAIN);
    lv_obj_set_style_text_color(roller, COLOR_MUTED_TEXT, LV_PART_MAIN);
    lv_obj_set_style_border_width(roller, 0, LV_PART_MAIN);

    // selected
    lv_obj_set_style_bg_color(roller, COLOR_PRIMARY, LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller, COLOR_TEXT, LV_PART_SELECTED);
    lv_obj_set_style_radius(roller, 4, LV_PART_SELECTED);


    return roller;
}

int find_roller_selected_index(const char* options, const char* selected) {
    int index = 0;
    const char* start = options;
    const char* found = strstr(options, selected);

    // Find the position of the selected string in the options
    while (start < found) {
        if (*start == '\n') {
            index++;
        }
        start++;
    }

    return index;
}

lv_obj_t* get_button(lv_obj_t* parent, char* buttonTxt) {
    lv_obj_t* button = lv_btn_create(parent);
    lv_obj_set_height(button, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(button, COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(button, 0, LV_PART_MAIN);

    lv_obj_t* buttonLabel = lv_label_create(button);
    lv_label_set_text(buttonLabel, buttonTxt);
    lv_obj_set_style_text_color(buttonLabel, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_center(buttonLabel);

    return button;
}