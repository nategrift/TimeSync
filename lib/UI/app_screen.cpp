#include "app_screen.h"

static uint32_t press_time = 0;
static lv_style_t style; // Style for the circle border
static lv_anim_t anim; // Animation object

lv_color_t grey_color = lv_color_hex(0x0d0d0d);
lv_color_t green_color = lv_color_hex(0x2e402c);

void init_circle_style() {
    lv_style_init(&style);
    lv_style_set_radius(&style, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&style, 0); // Start with no border
    lv_style_set_border_color(&style, grey_color);
    lv_style_set_border_opa(&style, LV_OPA_COVER); // Full opacity
    lv_style_set_border_side(&style, LV_BORDER_SIDE_FULL); // Apply border on all sides for a circle
}

void border_anim_cb(void * var, int32_t value) {
    uint32_t press_duration = lv_tick_elaps(press_time);

    lv_obj_t * obj = (lv_obj_t *)var;
    lv_obj_set_style_border_width(obj, value, LV_PART_MAIN);

    if (press_duration >= LONG_PRESS_THRESHOLD) {
        // Handle long press action and change color to light green
        lv_obj_set_style_border_color(obj, green_color, LV_PART_MAIN);
    }
}

void start_border_animation(lv_obj_t * obj) {
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, obj);
    lv_anim_set_values(&anim, 0, MAX_BORDER_WIDTH);
    lv_anim_set_time(&anim, ANIMATION_DURATION);
    lv_anim_set_delay(&anim, ANIMATION_START_DELAY);
    lv_anim_set_exec_cb(&anim, border_anim_cb);
    lv_anim_start(&anim);
}

void press_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    AppManager* appManager = (AppManager*)lv_event_get_user_data(e);

    if (code == LV_EVENT_PRESSED) {
        // Record the time when the button was pressed
        press_time = lv_tick_get();
        // Start the border animation
        start_border_animation(obj);
    } else if (code == LV_EVENT_RELEASED) {
        // Calculate the press duration
        uint32_t press_duration = lv_tick_elaps(press_time);

        if (press_duration >= LONG_PRESS_THRESHOLD) {
           if (appManager) {
                appManager->launchApp("AppSelector");
            }
        }

        // reset the border
        lv_obj_set_style_border_color(obj, grey_color, LV_PART_MAIN);
        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);

        // Stop any ongoing animation when released
        lv_anim_del(obj, border_anim_cb);

        // Reset press time
        press_time = 0;
    }
}

lv_obj_t* get_app_container(AppManager& appManager) {
    // Assume the display is a circle with a radius equal to half of the horizontal resolution
    int radius = LV_HOR_RES / 2;

    lv_obj_t* screenObj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screenObj, 2 * radius, 2 * radius); // Set size to be a circle
    lv_obj_align(screenObj, LV_ALIGN_CENTER, 0, 0); // Center the circle on the screen
    lv_obj_set_style_bg_color(screenObj, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_radius(screenObj, LV_RADIUS_CIRCLE, LV_PART_MAIN); // Set object to be circular
    lv_obj_add_event_cb(screenObj, press_event_handler, LV_EVENT_ALL, &appManager);

    lv_obj_set_style_border_width(screenObj, 0, LV_PART_MAIN); // Start with no border
    lv_obj_set_style_pad_all(screenObj, 0, LV_PART_MAIN);

    // Apply the initialized circle style
    init_circle_style();
    lv_obj_add_style(screenObj, &style, 0);

    return screenObj;
}
