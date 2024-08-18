#include "MotionDriver.h"
#include "esp_log.h"

static const char *TAG = "MotionDriver";

MotionDriver::MotionDriver() {}

MotionDriver::~MotionDriver() {}

esp_err_t MotionDriver::init() {
    esp_err_t ret;

    uint8_t accelConfig = 0b00100110;  // CTRL2: aFS = 010 (±8g), aODR = 0110 (125 Hz)
    ret = writeRegister(0x03, &accelConfig, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure accelerometer");
        return ret;
    }

    uint8_t gyroConfig = 0b10000101;  // CTRL3: gFS = 100 (±256 dps), gODR = 0101 (224.2 Hz)
    ret = writeRegister(0x04, &gyroConfig, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure gyroscope");
        return ret;
    }

    uint8_t enableSensors = 0b00000011;  // CTRL7: gEN = 1, aEN = 1
    ret = writeRegister(0x08, &enableSensors, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable sensors");
        return ret;
    }

    ESP_LOGI(TAG, "Motion driver initialized successfully");
    return ESP_OK;
}

esp_err_t MotionDriver::readGyroscope(float &x, float &y, float &z) {
    uint8_t data[6];
    esp_err_t ret = readRegister(0x3B, data, 6); // 0x3B is the starting register address for gyroscope data
    if (ret == ESP_OK) {
        int16_t raw_x = (data[1] << 8) | data[0];
        int16_t raw_y = (data[3] << 8) | data[2];
        int16_t raw_z = (data[5] << 8) | data[4];
        x = raw_x * (256.0f / 32768.0f); // Convert to degrees per second
        y = raw_y * (256.0f / 32768.0f);
        z = raw_z * (256.0f / 32768.0f);
    } else {
        ESP_LOGE(TAG, "Failed to read gyroscope data");
    }
    return ret;
}

esp_err_t MotionDriver::readAccelerometer(float &x, float &y, float &z) {
    uint8_t data[6];
    esp_err_t ret = readRegister(0x35, data, 6); // 0x35 is the starting register address for accelerometer data
    if (ret == ESP_OK) {
        int16_t raw_x = (data[1] << 8) | data[0];
        int16_t raw_y = (data[3] << 8) | data[2];
        int16_t raw_z = (data[5] << 8) | data[4];
        x = raw_x * (8.0f / 32768.0f); // Convert to g (gravitational acceleration)
        y = raw_y * (8.0f / 32768.0f);
        z = raw_z * (8.0f / 32768.0f);
    } else {
        ESP_LOGE(TAG, "Failed to read accelerometer data");
    }
    return ret;
}


esp_err_t MotionDriver::readRegister(uint8_t reg, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (QMI8658_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (QMI8658_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t MotionDriver::writeRegister(uint8_t reg, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (QMI8658_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}
