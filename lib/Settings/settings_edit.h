#ifndef SETTINGS_EDIT_UI_H
#define SETTINGS_EDIT_UI_H

#include "lvgl.h"
#include "Settings.h"
#include "esp_log.h"
#include "app_screen.h"
#include "ui_components.h"
#include "esp_system.h"

extern lv_obj_t* edit_screen;
extern lv_obj_t* previous_screen;

std::string create_roller_options(int start, int end);

lv_obj_t* get_settings_edit_screen(Setting* setting, lv_obj_t* previous_scr);
lv_obj_t* create_int_edit_screen(Setting* setting, lv_obj_t* previous_scr);
lv_obj_t* create_time_edit_screen(Setting* setting, lv_obj_t* previous_scr);
lv_obj_t* create_date_edit_screen(Setting* setting, lv_obj_t* previous_scr);
lv_obj_t* create_button_edit_screen(Setting* setting, lv_obj_t* previous_scr);
lv_obj_t* create_bool_edit_screen(Setting* setting, lv_obj_t* previous_scr);
lv_obj_t* create_string_edit_screen(Setting* setting, lv_obj_t* previous_scr);

void open_edit_screen(Setting* setting);

#endif // SETTINGS_EDIT_UI_H