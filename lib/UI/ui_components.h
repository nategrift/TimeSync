#ifndef DEFAULT_STYLES_H
#define DEFAULT_STYLES_H

#include "lvgl.h"

#define COLOR_PRIMARY_BRIGHT lv_color_hex(0xc40000)
#define COLOR_PRIMARY lv_color_hex(0x640000)
#define COLOR_PRIMARY_MUTED lv_color_hex(0x470000)
#define COLOR_SECONDARY lv_color_hex(0x441AC2)

#define COLOR_TEXT lv_color_hex(0xFFFFFF)
#define COLOR_TEXT_SECONDARY lv_color_hex(0xededed)
#define COLOR_MUTED_TEXT lv_color_hex(0x5C5252)

#define COLOR_INPUT_BACKGROUND lv_color_hex(0x181414)



#define COLOR_ERROR lv_color_hex(0x873535)
#define COLOR_WARNING lv_color_hex(0xab812c)
#define COLOR_SUCCESS lv_color_hex(0x398735)

lv_obj_t* get_default_roller(lv_obj_t* parent);
int find_roller_selected_index(const char* options, const char* selected);

lv_obj_t* get_button(lv_obj_t* parent, char* buttonTxt);


#endif // DEFAULT_STYLES_H