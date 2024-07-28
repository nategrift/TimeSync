#ifndef TOUCHDRIVER_H
#define TOUCHDRIVER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "lvgl.h"


struct TouchData {
    uint16_t x;
    uint16_t y;
    uint8_t gesture_id;
    bool touch_detected;
};

class TouchDriver {
public:
    TouchDriver();
    ~TouchDriver();

    esp_err_t init();
    TouchData getTouchCoordinates();
    void lvglRead(lv_indev_drv_t *drv, lv_indev_data_t *data);

private:
    static const uint8_t I2C_ADDR = 0x15; // I2C address of the CST816S
    esp_err_t reset();
    esp_err_t i2cMasterInit();
    esp_err_t readRegister(uint8_t reg, uint8_t *data, size_t len);
    esp_err_t writeRegister(uint8_t reg, uint8_t *data, size_t len);
};

#endif // TOUCHDRIVER_H
