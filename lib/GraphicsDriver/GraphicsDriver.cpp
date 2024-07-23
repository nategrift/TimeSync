
#include "GraphicsDriver.h"
#define TAG "GraphicsDriver"

#include "TouchDriver.h"

GraphicsDriver::GraphicsDriver() {
    // Constructor
}

void GraphicsDriver::init() {
    // TouchDriver touchDriver;
    // touchDriver.init();
    
    gc9a01_displayInit();
    lvglDisplayConfig();

    // touchDriver.initTouchForGraphics();

    // // Create LVGL task
    xTaskCreatePinnedToCore(lvgl_task, "lvgl_task", 10000, NULL, 4, NULL, 1);
}

void GraphicsDriver::lvgl_task(void *arg) {
    ESP_LOGI(TAG, "Starting LVGL task");
    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void event_cb(lv_event_t * e)
{
    LV_LOG_USER("Clicked");

    static uint32_t cnt = 1;
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);
    lv_label_set_text_fmt(label, "%"LV_PRIu32, cnt);
    cnt++;
}


void GraphicsDriver::addTextToCenter(const char* text) {
    /*Change the active screen's background color*/
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), LV_PART_MAIN);

    /*Create a white label, set its text and align it to the center*/
    // lv_obj_t * label = lv_label_create(lv_scr_act());
    // lv_label_set_text(label, text);
    // lv_obj_set_style_text_color(lv_scr_act(), lv_color_hex(0xffffff), LV_PART_MAIN);
    // lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 100, 50);
    lv_obj_center(btn);
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, "Click me!");
    lv_obj_center(label);

    

}


void GraphicsDriver::setupTouchDriver(TouchDriver &touchDriver) {
    // Initialize LVGL input device driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = [](lv_indev_drv_t *drv, lv_indev_data_t *data) {
        TouchDriver *touch = static_cast<TouchDriver*>(drv->user_data);
        touch->lvglRead(drv, data);
    };
    indev_drv.user_data = &touchDriver;
    lv_indev_t *my_indev = lv_indev_drv_register(&indev_drv);
}