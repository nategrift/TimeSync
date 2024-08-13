#include "app_screen.h"

#include "esp_log.h"
#include "AwakeManager.h"

static uint32_t press_time = 0;
static lv_style_t style; // Style for the circle border
static lv_anim_t anim; // Animation object

lv_color_t grey_color = lv_color_hex(0x0d0d0d);
lv_color_t green_color = lv_color_hex(0x2e402c);

// Variables to track touch position
static lv_point_t start_pos;
static lv_point_t end_pos;

static const char *TAG = "app_screen";

// Gesture thresholds
#define SWIPE_THRESHOLD 30

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

void gesture_action(AppManager* appManager, lv_dir_t gesture) {
    switch (gesture) {
        case LV_DIR_TOP:
            ESP_LOGI(TAG, "SwipeUp");
            break;
        case LV_DIR_BOTTOM:
            ESP_LOGI(TAG, "SwipeDown");
            AwakeManager::sleepDevice();
            break;
        case LV_DIR_LEFT:
            ESP_LOGI(TAG, "SwipeLeft");
            break;
        case LV_DIR_RIGHT:
            ESP_LOGI(TAG, "SwipeRight");
            break;
        default:
            if (appManager) {
                appManager->launchApp("AppSelector");
            }
            break;
    }
}

lv_dir_t detect_gesture(const lv_point_t& start, const lv_point_t& end) {
    int dx = end.x - start.x;
    int dy = end.y - start.y;

    if (abs(dx) > SWIPE_THRESHOLD || abs(dy) > SWIPE_THRESHOLD) {
        if (abs(dx) > abs(dy)) {
            // Horizontal swipe
            return (dx > 0) ? LV_DIR_RIGHT : LV_DIR_LEFT;
        } else {
            // Vertical swipe
            return (dy > 0) ? LV_DIR_BOTTOM : LV_DIR_TOP;
        }
    }
    return LV_DIR_NONE;
}

void press_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    AppManager* appManager = (AppManager*)lv_event_get_user_data(e);

    if (code == LV_EVENT_PRESSED) {
        // Record the time and position when the button was pressed
        press_time = lv_tick_get();
        lv_indev_t* indev = lv_indev_get_act();
        lv_indev_get_point(indev, &start_pos);

        // Start the border animation
        start_border_animation(obj);
    } else if (code == LV_EVENT_RELEASED) {
        // Calculate the press duration and position
        uint32_t press_duration = lv_tick_elaps(press_time);
        lv_indev_t* indev = lv_indev_get_act();
        lv_indev_get_point(indev, &end_pos);

        lv_dir_t gesture = detect_gesture(start_pos, end_pos);

        // Reset the border
        lv_obj_set_style_border_color(obj, grey_color, LV_PART_MAIN);
        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);

        // Stop any ongoing animation when released
        lv_anim_del(obj, border_anim_cb);

        // Reset press time
        press_time = 0;

        // HANDLE ACTION
        if (press_duration >= LONG_PRESS_THRESHOLD) {
            gesture_action(appManager, gesture);
        }
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
    lv_obj_set_style_border_width(screenObj, 0, LV_PART_MAIN); // Start with no border
    lv_obj_set_style_pad_all(screenObj, 0, LV_PART_MAIN);
    
    lv_obj_add_event_cb(screenObj, press_event_handler, LV_EVENT_ALL, &appManager);

    // Apply the initialized circle style
    init_circle_style();
    lv_obj_add_style(screenObj, &style, 0);

    return screenObj;
}

lv_obj_t* get_blank_screen() {
    lv_obj_t* screenObj = lv_obj_create(NULL);
    lv_obj_set_size(screenObj, LV_HOR_RES, LV_VER_RES);
    lv_obj_center(screenObj);
    lv_obj_set_style_bg_color(screenObj, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_border_width(screenObj, 0, LV_PART_MAIN); // Start with no border
    lv_obj_set_style_pad_all(screenObj, 0, LV_PART_MAIN);

    return screenObj;
}