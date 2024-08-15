#ifndef GRAPHICS_DRIVER_H
#define GRAPHICS_DRIVER_H

#include <string>
#include <map>
#include <memory>
#include <vector>
#include "lvgl.h"

extern "C" {
#include "display.h"
#include "gc9a01.h"
}

#include "TouchDriver.h"

#define PWM_CHANNEL LEDC_CHANNEL_0
#define PWM_TIMER LEDC_TIMER_0
#define PWM_FREQUENCY 5000      // PWM frequency of 5 kHz
#define MAX_DUTY_CYCLE 8191     // 13-bit resolution (2^13 - 1)

class GraphicsDriver {
private:
    static void lvgl_task(void *arg);

public:
    GraphicsDriver();
    void init();
    void setupTouchDriver(TouchDriver &touchDriver);
    static void init_backlight_pwm();
    static void set_backlight_brightness(uint8_t brightness);
};

#endif // GRAPHICS_DRIVER_H
