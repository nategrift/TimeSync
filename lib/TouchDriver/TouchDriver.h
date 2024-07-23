// TouchDriver.h
#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include <string>
#include <map>
#include <memory>
#include <vector>
#include "lvgl.h"
#include "driver/i2c.h"
#include "esp_lcd_touch_cst816s.h"

class TouchDriver {
public:
    TouchDriver();
    ~TouchDriver();
    void init();
    void initTouchForGraphics();

private:
    static void touchpadRead(lv_indev_drv_t *drv, lv_indev_data_t *data);
    static void touchCallback(esp_lcd_touch_handle_t tp);
    
    static const gpio_num_t I2C_SCL = GPIO_NUM_7;
    static const gpio_num_t I2C_SDA = GPIO_NUM_6;
    static const i2c_port_t I2C_NUM = I2C_NUM_0;
    static const gpio_num_t RST_GPIO = GPIO_NUM_13;
    static const gpio_num_t INT_GPIO = GPIO_NUM_5;
    

};

#endif // TOUCH_DRIVER_H
