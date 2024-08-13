#ifndef APP_SCREEN_H
#define APP_SCREEN_H

#include "lvgl.h"
#include "AppManager.h"

#define LONG_PRESS_THRESHOLD 600 // Long press threshold in ms
#define ANIMATION_START_DELAY 100 // Del
#define MAX_BORDER_WIDTH 10 // Maximum border thickness
#define ANIMATION_DURATION 500 // Duration of the animation in ms = LONG_PRESS_THRESHOLD - ANIMATION_START_DELAY

extern lv_color_t grey_color;
extern lv_color_t green_color;

void init_circle_style();
void border_anim_cb(void * var, int32_t value);
void start_border_animation(lv_obj_t * obj);
void press_event_handler(lv_event_t * e);

lv_obj_t* get_app_container(AppManager& appManager);
lv_obj_t* get_blank_screen();

#endif // APP_SCREEN_Hay before the animation starts in ms