#pragma once

#include "driver/i2c.h"
#include "esp_err.h"

#define QMI8658_I2C_ADDR 0x6B
#define I2C_PORT I2C_NUM_0

class MotionDriver {
public:
    MotionDriver();
    ~MotionDriver();

    static esp_err_t init();
    static esp_err_t enableGyroAndAcc();
    static esp_err_t readGyroscope(float &x, float &y, float &z);
    static esp_err_t readAccelerometer(float &x, float &y, float &z);

    static esp_err_t resetStepCount();
    static esp_err_t readStepCount(uint32_t &stepCount);
    static esp_err_t enablePedometer();
    static esp_err_t configurePedometer();
    static esp_err_t isPedometerRunning(bool &isRunning);


private:
    static esp_err_t readRegister(uint8_t reg, uint8_t *data, size_t len);
    static esp_err_t writeRegister(uint8_t reg, uint8_t *data, size_t len);
};
