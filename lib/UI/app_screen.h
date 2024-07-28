#include "lvgl.h"
#include "AppManager.h"


#define LONG_PRESS_THRESHOLD 1000

static void press_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    AppManager* appManager = (AppManager*)lv_event_get_user_data(e);

    uint32_t press_time = 0;
    if (code == LV_EVENT_PRESSED) {
        // Record the time when the button was pressed
        press_time = lv_tick_get();
    } else if (code == LV_EVENT_RELEASED) {
        // Calculate the press duration
        uint32_t press_duration = lv_tick_elaps(press_time);
        
        if (press_duration >= LONG_PRESS_THRESHOLD) {
            // Handle long press action
            if (appManager) {
                appManager->launchApp("AppSelector");
            }
        } else {
            // Handle short press action if necessary
        }
    }
}


static lv_obj_t* get_app_container(AppManager& appManager) {
    lv_obj_t* screenObj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screenObj, LV_HOR_RES, LV_VER_RES);
    lv_obj_align(screenObj, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(screenObj, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_add_event_cb(screenObj, press_event_handler, LV_EVENT_ALL, &appManager);

    lv_obj_set_style_border_width(screenObj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screenObj, 0, LV_PART_MAIN);

    return screenObj;
}
