#ifndef DEFAULT_STYLES_H
#define DEFAULT_STYLES_H

#include "lvgl.h"

#define COLOR_PRIMARY lv_color_hex(0x640000)

#define COLOR_TEXT lv_color_hex(0xFFFFFF)
#define COLOR_MUTED_TEXT lv_color_hex(0x5C5252)

#define COLOR_INPUT_BACKGROUND lv_color_hex(0x181414)

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


#endif // DEFAULT_STYLES_H