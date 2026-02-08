#include "MotionDebug.h"
#include "VibrationDriver.h"
#include "app_screen.h"
#include "ui_components.h"

lv_obj_t* MotionDebug::textArea = nullptr;
lv_obj_t* MotionDebug::circle = nullptr;
lv_obj_t* MotionDebug::refreshBtn = nullptr;

MotionDebug::MotionDebug(AppManager& manager) 
    : IApp("MotionDebug"), appManager(manager), screenObj(NULL), updateTaskHandle(NULL), isRunning(false) {
}

MotionDebug::~MotionDebug() {}


void MotionDebug::launch() {
    screenObj = get_app_container(appManager);

    // Create title
    lv_obj_t* titleLabel = lv_label_create(screenObj);
    lv_label_set_text(titleLabel, "Motion Debug");
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 10);

    // Style for title
    static lv_style_t style_title;
    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, lv_color_hex(0x00FF00));
    lv_obj_add_style(titleLabel, &style_title, 0);

    // Create scrollable text area
    textArea = lv_textarea_create(screenObj);
    lv_obj_set_size(textArea, 220, 160);
    lv_obj_align(textArea, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_text(textArea, "Loading...");
    lv_obj_add_flag(textArea, LV_OBJ_CLASS_EDITABLE_FALSE);

    // Style for text area
    static lv_style_t style_ta;
    lv_style_init(&style_ta);
    lv_style_set_text_color(&style_ta, lv_color_hex(0xFFFFFF));
    lv_style_set_bg_color(&style_ta, lv_color_hex(0x000000));
    lv_obj_add_style(textArea, &style_ta, 0);

    MotionDebug::circle = lv_obj_create(screenObj); // Create circle on app screen instead of active screen
    lv_obj_set_size( MotionDebug::circle, 20, 20);
    lv_obj_set_style_radius( MotionDebug::circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color( MotionDebug::circle, lv_color_hex(0xFF0000), 0);
    lv_obj_add_flag(MotionDebug::circle, LV_OBJ_FLAG_FLOATING);
    lv_obj_move_foreground( MotionDebug::circle);
    lv_obj_set_pos( MotionDebug::circle, 120, 120);

    // Create refresh button
    MotionDebug::refreshBtn = get_button(screenObj, "Reset");
    lv_obj_align(MotionDebug::refreshBtn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(MotionDebug::refreshBtn, refresh_event_handler, LV_EVENT_CLICKED, this);

    lv_timer_create(MotionDebug::updateDisplay, 50, NULL);
}

void MotionDebug::refresh_event_handler(lv_event_t *event) {
    MotionDriver::resetStepCount();
}

void MotionDebug::updateDisplay(lv_timer_t *timer) {
    // Pre-allocate string buffer to avoid reallocations
    std::string info;
    
    // Read accelerometer data
    float acc_x, acc_y, acc_z;
    if (MotionDriver::readAccelerometer(acc_x, acc_y, acc_z) == ESP_OK) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "A (X): %.2f\nA (Y): %.2f\nA (Z): %.2f\n", 
                acc_x, acc_y, acc_z);
        info += buffer;
    }

    // Read gyroscope data
    float gyro_x, gyro_y, gyro_z;
    if (MotionDriver::readGyroscope(gyro_x, gyro_y, gyro_z) == ESP_OK) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "G (X): %.2f\nG (Y): %.2f\nG (Z): %.2f\n",
                gyro_x, gyro_y, gyro_z);
        info += buffer;
    }

    // Calculate new position by mapping -1 to 1 range to screen coordinates
    // For X: -1 -> 0, 0 -> 120, 1 -> 240
    // For Y: -1 -> 0, 0 -> 120, 1 -> 240
    int x_pos = (int)((acc_x + 1.0f) * (LV_HOR_RES / 2));
    int y_pos = (int)((acc_y + 1.0f) * (LV_VER_RES / 2));
    // Cap x_pos and y_pos between 0 and 240 using std::clamp
    x_pos = std::clamp(x_pos, 0, 240);
    y_pos = std::clamp(y_pos, 0, 240);

    // Update circle position only if the object is still valid
    if (MotionDebug::circle && lv_obj_is_valid( MotionDebug::circle)) {
        lv_obj_set_pos(MotionDebug::circle, x_pos, y_pos);
    } else {
        ESP_LOGE("MotionDebug", "Circle object is invalid");
    }

    // // Read pedometer data
    bool pedometerRunning;
    uint32_t stepCount;
    
    if (MotionDriver::isPedometerRunning(pedometerRunning) == ESP_OK) {
        info += "Pedometer: ";
        info += (pedometerRunning ? "Running" : "Stopped");
    }
    
    if (MotionDriver::readStepCount(stepCount) == ESP_OK) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "  Steps: %lu\n", stepCount);
        info += buffer;
    }

    if (textArea && lv_obj_is_valid(textArea)) {
        lv_textarea_set_text(textArea, info.c_str());
    }
}

void MotionDebug::close() {
    // Stop the update task
    // if (isRunning) {
    //     isRunning = false;
    //     if (updateTaskHandle != NULL) {
    //         vTaskDelay(pdMS_TO_TICKS(300)); // Wait for last iteration
    //         updateTaskHandle = NULL;
    //     }
    // }
}

void MotionDebug::backgroundActivity() {
    // Not used
}