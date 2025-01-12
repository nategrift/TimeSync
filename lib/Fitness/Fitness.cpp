#include "Fitness.h"
#include <esp_log.h>
#include "app_screen.h"
#include "ui_components.h"
#include "ui_components.h"
#include "ConfigManager.h"
#include "FitnessManager.h"


const int STEPS_GOAL = 4000;
const int STEPS_HOURLY_GOAL = 200;
static int last_steps = 0;
static int last_hourly_steps = 0;

Fitness::Fitness(AppManager& manager) 
    : IApp("Fitness"), appManager(manager), screenObj(NULL) {}


Fitness::~Fitness() {
    
}

static void set_arc_angle(void* obj, int32_t v)
{
    lv_arc_set_value((lv_obj_t*)obj, v);
}


void Fitness::launch() {

    screenObj = get_app_container(appManager);
    stepsLabel = lv_label_create(screenObj);
    lv_label_set_text(stepsLabel, "0");
    lv_obj_align(stepsLabel, LV_ALIGN_CENTER, 0, -45); // Align time in the center

    // Set the style for the time
    static lv_style_t style_steps;
    lv_style_init(&style_steps);
    lv_style_set_text_color(&style_steps, lv_color_hex(0xFFFFFF)); // White color
    lv_style_set_text_font(&style_steps, &lv_font_montserrat_48); // Increase font size
    lv_obj_add_style(stepsLabel, &style_steps, 0);

    lv_obj_t* goalLabel = lv_label_create(screenObj);
    lv_label_set_text(goalLabel, ("Goal: " + std::to_string(STEPS_GOAL)).c_str());
    lv_obj_align(goalLabel, LV_ALIGN_CENTER, 0, 26);

    // Set the style for the goal
    static lv_style_t goal_style;
    lv_style_init(&goal_style);
    lv_style_set_text_color(&goal_style, COLOR_MUTED_TEXT);
    lv_style_set_text_font(&goal_style, &lv_font_montserrat_16);
    lv_obj_add_style(goalLabel, &goal_style, 0);

    hourlyStepsLabel = lv_label_create(screenObj);
    lv_label_set_text(hourlyStepsLabel, "Hour: 0");
    lv_obj_align(hourlyStepsLabel, LV_ALIGN_CENTER, 0, 0);

    // Set the style for the hourly steps
    static lv_style_t hourly_style;
    lv_style_init(&hourly_style);
    lv_style_set_text_color(&hourly_style, COLOR_TEXT_SECONDARY);
    lv_style_set_text_font(&hourly_style, &lv_font_montserrat_16);
    lv_obj_add_style(hourlyStepsLabel, &hourly_style, 0);

    stepsGoalArc = lv_arc_create(screenObj);
    lv_obj_set_size(stepsGoalArc, LV_HOR_RES - 30, LV_HOR_RES - 30);
    lv_obj_remove_style(stepsGoalArc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(stepsGoalArc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(stepsGoalArc);
    lv_arc_set_range(stepsGoalArc, 0, STEPS_GOAL);
    lv_arc_set_rotation(stepsGoalArc, 270);
    lv_arc_set_mode(stepsGoalArc, LV_ARC_MODE_NORMAL);
    lv_arc_set_bg_angles(stepsGoalArc, 80, 280);
    lv_obj_set_style_arc_width(stepsGoalArc, 15, LV_PART_MAIN);
    lv_obj_set_style_arc_width(stepsGoalArc, 15, LV_PART_INDICATOR);
    // Set the style for the arc background (unfilled part)
    static lv_style_t style_arc_bg;
    lv_style_init(&style_arc_bg);
    lv_style_set_arc_color(&style_arc_bg, COLOR_INPUT_BACKGROUND);
    lv_obj_add_style(stepsGoalArc, &style_arc_bg, LV_PART_MAIN);

    // Set the style for the arc indicator (filled part)
    static lv_style_t style_arc_ind;
    lv_style_init(&style_arc_ind);
    lv_style_set_arc_color(&style_arc_ind, COLOR_SECONDARY);
    lv_obj_add_style(stepsGoalArc, &style_arc_ind, LV_PART_INDICATOR);

    stepsHourlyGoalArc = lv_arc_create(screenObj);
    lv_obj_set_size(stepsHourlyGoalArc, LV_HOR_RES - 80, LV_HOR_RES - 80);
    lv_obj_remove_style(stepsHourlyGoalArc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(stepsHourlyGoalArc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(stepsHourlyGoalArc);
    lv_arc_set_range(stepsHourlyGoalArc, 0, STEPS_HOURLY_GOAL);
    lv_arc_set_rotation(stepsHourlyGoalArc, 270);
    lv_arc_set_mode(stepsHourlyGoalArc, LV_ARC_MODE_NORMAL);
    lv_arc_set_bg_angles(stepsHourlyGoalArc, 120, 240);
    lv_obj_set_style_arc_width(stepsHourlyGoalArc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(stepsHourlyGoalArc, 10, LV_PART_INDICATOR);
    // Set the style for the arc background (unfilled part)
    lv_obj_add_style(stepsHourlyGoalArc, &style_arc_bg, LV_PART_MAIN);

    // Set the style for the arc indicator (filled part)
    static lv_style_t style_hourly_arc_ind;
    lv_style_init(&style_hourly_arc_ind);
    lv_style_set_arc_color(&style_hourly_arc_ind, COLOR_PRIMARY_BRIGHT);
    lv_obj_add_style(stepsHourlyGoalArc, &style_hourly_arc_ind, LV_PART_INDICATOR);

    // Create the FreeRTOS task
    xTaskCreatePinnedToCore(
        stepsTaskWrapper,    // Static task function wrapper
        "steps_task",        // Task name
        2048,               // Stack size
        this,               // Parameter passed to task (this pointer)
        5,                  // Task priority
        &stepsTaskHandle,   // Task handle
        0                   // Core ID
    );
}

void Fitness::close() {
    // Delete the task when closing the app
    if (stepsTaskHandle != NULL) {
        vTaskDelete(stepsTaskHandle);
        stepsTaskHandle = NULL;
    }

    if (screenObj != NULL) {
        lv_obj_del(screenObj);
        screenObj = NULL;
    }
    last_steps = 0;
    last_hourly_steps = 0;
}

// Static wrapper function to call the member function
void Fitness::stepsTaskWrapper(void* parameter) {
    Fitness* fitness = static_cast<Fitness*>(parameter);
    while (1) {
        int steps = ConfigManager::getConfigInt(FitnessManager::KEY, FitnessManager::DAILY_STEPS_KEY);
        int hourlySteps = ConfigManager::getConfigInt(FitnessManager::KEY, FitnessManager::CURRENT_HOUR_STEPS_KEY);

        lv_label_set_text(fitness->stepsLabel, std::to_string(steps).c_str());
        lv_obj_align(fitness->stepsLabel, LV_ALIGN_CENTER, 0, -40);

        lv_label_set_text(fitness->hourlyStepsLabel, ("Hour: " + std::to_string(hourlySteps)).c_str());
        lv_obj_align(fitness->hourlyStepsLabel, LV_ALIGN_CENTER, 0, 0);
        
        if (steps != last_steps) {
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, fitness->stepsGoalArc);
            lv_anim_set_exec_cb(&a, set_arc_angle);
            lv_anim_set_time(&a, 1000);
            lv_anim_set_values(&a, last_steps, steps);
            lv_anim_start(&a);

            last_steps = steps;
        }

        if (hourlySteps != last_hourly_steps) {
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, fitness->stepsHourlyGoalArc);
            lv_anim_set_exec_cb(&a, set_arc_angle);
            lv_anim_set_time(&a, 1000);
            lv_anim_set_values(&a, last_hourly_steps, hourlySteps);
            lv_anim_start(&a);

            last_hourly_steps = hourlySteps;
        }

    
        vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds delay
    }
}

void Fitness::backgroundActivity() {
    // not used
}