#pragma once

#include "driver/i2c.h"
#include "esp_err.h"

#define QMI8658_I2C_ADDR 0x6B  // Replace with the correct I2C address if needed
#define I2C_PORT I2C_NUM_0

class MotionDriver {
public:
    MotionDriver();
    ~MotionDriver();

    esp_err_t init();
    esp_err_t readGyroscope(float &x, float &y, float &z);
    esp_err_t readAccelerometer(float &x, float &y, float &z);

private:
    esp_err_t readRegister(uint8_t reg, uint8_t *data, size_t len);
    esp_err_t writeRegister(uint8_t reg, uint8_t *data, size_t len);
};
