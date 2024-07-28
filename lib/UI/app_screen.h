#include "lvgl.h"
#include "AppManager.h"


static void press_event_handler(lv_event_t * e);
static lv_obj_t* get_app_container(AppManager& appManager);