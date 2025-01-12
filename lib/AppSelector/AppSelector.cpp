#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "AppSelector.h"
#include "lvgl.h"
#include <string>
#include "esp_log.h"

static const char* TAG = "AppSelector";

AppSelector::AppSelector(AppManager& manager) 
    : IApp("AppSelector"), // Initialize with the app name
      appManager(manager), 
      screenObj(nullptr) {
}

AppSelector::~AppSelector() {

}

static bool is_scrolling = false;
static lv_timer_t *scroll_timer = nullptr;

static void scroll_event_cb(lv_event_t * e)
{
    if (scroll_timer != nullptr) {
        lv_timer_del(scroll_timer);
        scroll_timer = nullptr;
    }

    is_scrolling = true;
    lv_obj_t * cont = (lv_obj_t *)lv_event_get_target(e);

    lv_area_t cont_a;
    lv_obj_get_coords(cont, &cont_a);
    int32_t cont_y_center = cont_a.y1 + lv_area_get_height(&cont_a) / 2;

    int32_t r = lv_obj_get_height(cont) * 7 / 10;
    uint32_t child_cnt = lv_obj_get_child_cnt(cont);

    for(uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(cont, i);
        lv_area_t child_a;
        lv_obj_get_coords(child, &child_a);

        int32_t child_y_center = child_a.y1 + lv_area_get_height(&child_a) / 2;
        int32_t diff_y = LV_ABS(child_y_center - cont_y_center);

        int32_t x;
        if (diff_y < r) {
            // Prepare the result structure for the square root calculation
            lv_sqrt_res_t res;
            
            // Calculate the square root
            lv_sqrt((r * r) - (diff_y * diff_y), &res, 0x8000); // Use appropriate mask
            
            // Get the integer part of the square root
            x = r - res.i;
        } else {
            x = r;
        }

        // Apply transformations
        lv_obj_set_style_translate_x(child, x, 0);

        // Calculate and apply opacity
        lv_opa_t opa = lv_map(x, 0, r, LV_OPA_TRANSP, LV_OPA_COVER);
        lv_obj_set_style_opa(child, LV_OPA_COVER - opa, 0);
    }
    
    scroll_timer = lv_timer_create([](lv_timer_t * timer) {
        is_scrolling = false;
        scroll_timer = nullptr; // Clear the reference to indicate the timer is done
    }, 200, nullptr);
}

void AppSelector::launch() {
// Create a new LVGL object
    screenObj = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(screenObj, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_size(screenObj, LV_HOR_RES, LV_VER_RES);
    lv_obj_center(screenObj);
    lv_obj_set_flex_flow(screenObj, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_event_cb(screenObj, scroll_event_cb, LV_EVENT_SCROLL, NULL);
    lv_obj_set_style_radius(screenObj, LV_RADIUS_CIRCLE, 3);
    lv_obj_set_style_clip_corner(screenObj, true, 0);
    lv_obj_set_scroll_dir(screenObj, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(screenObj, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(screenObj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(screenObj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screenObj, 0, LV_PART_MAIN);

    // Define a common style for list buttons
    static lv_style_t style_btn;
    lv_style_init(&style_btn);
    lv_style_set_bg_color(&style_btn, lv_color_hex(0x050505));
    lv_style_set_border_width(&style_btn, 0);
    lv_style_set_pad_all(&style_btn, 20);
    lv_style_set_text_color(&style_btn, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_btn, &lv_font_montserrat_16);
    lv_style_set_img_recolor(&style_btn, lv_color_hex(0xFFFFFF));
    lv_style_set_img_recolor_opa(&style_btn, LV_OPA_100);


    lv_obj_t* btn_clock = lv_list_add_btn(screenObj, LV_SYMBOL_HOME, "Clock");
    lv_obj_add_style(btn_clock, &style_btn, LV_PART_MAIN);
    // lv_obj_t* btn_alarm = lv_list_add_btn(screenObj, LV_SYMBOL_BELL, "Alarm");
    // lv_obj_add_style(btn_alarm, &style_btn, LV_PART_MAIN);
    lv_obj_t* btn_stopwatch = lv_list_add_btn(screenObj, LV_SYMBOL_FILE, "StopWatch");
    lv_obj_add_style(btn_stopwatch, &style_btn, LV_PART_MAIN);
    lv_obj_t* btn_settings = lv_list_add_btn(screenObj, LV_SYMBOL_SETTINGS, "Settings");
    lv_obj_add_style(btn_settings, &style_btn, LV_PART_MAIN);
    lv_obj_t* btn_timer = lv_list_add_btn(screenObj, LV_SYMBOL_BELL, "Timer");
    lv_obj_add_style(btn_timer, &style_btn, LV_PART_MAIN);
    lv_obj_t* btn_fitness = lv_list_add_btn(screenObj, LV_SYMBOL_NEXT, "Fitness");
    lv_obj_add_style(btn_fitness, &style_btn, LV_PART_MAIN);

    // Add event handler for each button
    auto event_cb = [](lv_event_t* event) {
        lv_event_code_t code = lv_event_get_code(event);
        lv_obj_t* target = (lv_obj_t *)lv_event_get_target(event);

        if (code == LV_EVENT_CLICKED && !is_scrolling) {
            AppSelector* appSelector = reinterpret_cast<AppSelector*>(lv_event_get_user_data(event));
            const char* selected = lv_list_get_btn_text(appSelector->screenObj, target);
            ESP_LOGI("AppSelector", "AppSelector chooses app id of %s", selected);
            if (appSelector != nullptr) {
                std::string selectedApp(selected);
                appSelector->appManager.launchApp(selectedApp);
            }
        }
    };

        /*Update the buttons position manually for first*/
    lv_obj_send_event(screenObj, LV_EVENT_SCROLL, NULL);

    /*Be sure the fist button is in the middle*/
    lv_obj_scroll_to_view(lv_obj_get_child(screenObj, 0), LV_ANIM_OFF);

    lv_obj_add_event_cb(btn_clock, event_cb, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(btn_timer, event_cb, LV_EVENT_CLICKED, this);
    // lv_obj_add_event_cb(btn_alarm, event_cb, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(btn_stopwatch, event_cb, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(btn_settings, event_cb, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(btn_fitness, event_cb, LV_EVENT_CLICKED, this);
}

void AppSelector::close() {
    if (screenObj && lv_obj_is_valid(screenObj)) {
        lv_obj_del_async(screenObj);
        screenObj = nullptr;
    }
}

void AppSelector::backgroundActivity() {
    // Nothing
}
