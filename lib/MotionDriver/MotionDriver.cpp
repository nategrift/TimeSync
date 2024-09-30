#include "MotionDriver.h"
#include "esp_log.h"

static const char *TAG = "MotionDriver";

MotionDriver::MotionDriver() {}

MotionDriver::~MotionDriver() {}

esp_err_t MotionDriver::init() {
    esp_err_t ret;

    uint8_t disableSensors = 0b00000000; 
    ret = writeRegister(0x08, &disableSensors, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disable sensors");
        return ret;
    }

    ret = configurePedometer();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure pedometer");
        return ret;
    }

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

    return ESP_OK;  // Add this line to fix the control reaches end of non-void function error
}

esp_err_t MotionDriver::enableGyroAndAcc() {
    esp_err_t ret;

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


esp_err_t MotionDriver::configurePedometer() {
    esp_err_t ret;
    uint8_t data[2];

    // Step 1: Configure pedometer parameters (First CTRL9 Command)
    // ped_sample_cnt = 50 samples (0x0032) for 1 second window @ ODR = 50Hz
    data[0] = 0x32;  // ped_sample_cnt[7:0] = 50 samples
    data[1] = 0x00;  // ped_sample_cnt[15:8]
    ret = writeRegister(0x0A, data, 2);  // Write to CAL1_L and CAL1_H
    if (ret != ESP_OK) return ret;

    // ped_fix_peak2peak = 200mg (0x00CC in u6.10 format)
    data[0] = 0xCC;  // ped_fix_peak2peak[7:0] = 200mg
    data[1] = 0x00;  // ped_fix_peak2peak[15:8]
    ret = writeRegister(0x0C, data, 2);  // Write to CAL2_L and CAL2_H
    if (ret != ESP_OK) return ret;

    // ped_fix_peak = 100mg (0x0066 in u6.10 format)
    data[0] = 0x66;  // ped_fix_peak[7:0] = 100mg
    data[1] = 0x00;  // ped_fix_peak[15:8]
    ret = writeRegister(0x0E, data, 2);  // Write to CAL3_L and CAL3_H
    if (ret != ESP_OK) return ret;

    // First command to configure pedometer
    data[0] = 0x01;  // First CTRL9 command
    ret = writeRegister(0x10, data, 1);  // Write to CAL4_H
    if (ret != ESP_OK) return ret;

    // Send configuration command to CTRL9
    data[0] = 0x0D;  // CTRL_CMD_CONFIGURE_PEDOMETER
    ret = writeRegister(0x11, data, 1);  // Write to CTRL9
    if (ret != ESP_OK) return ret;

    // Step 2: Configure step timing parameters (Second CTRL9 Command)
    // ped_time_up = 200 samples (0x00C8, 4s at 50Hz)
    data[0] = 0xC8;  // ped_time_up[7:0] = 200 samples
    data[1] = 0x00;  // ped_time_up[15:8]
    ret = writeRegister(0x0A, data, 2);  // Write to CAL1_L and CAL1_H
    if (ret != ESP_OK) return ret;

    // ped_time_low = 20 samples (0x14, 0.4s at 50Hz)
    data[0] = 0x14;  // ped_time_low
    ret = writeRegister(0x0C, data, 1);  // Write to CAL2_L
    if (ret != ESP_OK) return ret;

    // ped_time_cnt_entry = 2 steps (0x02)
    data[0] = 0x02;  // ped_time_cnt_entry
    ret = writeRegister(0x0D, data, 1);  // Write to CAL2_H
    if (ret != ESP_OK) return ret;

    // Second command to finalize pedometer configuration
    data[0] = 0x02;  // Second CTRL9 command
    ret = writeRegister(0x10, data, 1);  // Write to CAL4_H
    if (ret != ESP_OK) return ret;

    // Send the second configuration command to CTRL9
    data[0] = 0x0D;  // CTRL_CMD_CONFIGURE_PEDOMETER
    ret = writeRegister(0x11, data, 1);  // Write to CTRL9
    if (ret != ESP_OK) return ret;

    return ESP_OK;
}

esp_err_t MotionDriver::enablePedometer() {
    // Sleep for 20ms to allow the pedometer configuration to settle
    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t data = 0x10;  // Enable pedometer (CTRL8.bit4 = 1)
    return writeRegister(0x09, &data, 1);  // Write to CTRL8 to enable pedometer
}

esp_err_t MotionDriver::isPedometerRunning(bool &isRunning) {
    uint8_t data;
    esp_err_t ret = readRegister(0x09, &data, 1);  // Read CTRL8 register
    if (ret == ESP_OK) {
        isRunning = (data & 0x10) ? true : false;  // Check if Pedo_EN (bit 4) is set
        ESP_LOGI(TAG, "Pedometer is %s", isRunning ? "running" : "stopped");
    } else {
        ESP_LOGE(TAG, "Failed to read pedometer status");
    }
    return ret;
}

esp_err_t MotionDriver::readStepCount(uint32_t &stepCount) {
    uint8_t data[3];  // Buffer to store the 3 bytes
    esp_err_t ret = readRegister(0x5A, data, 3);  // Read STEP_CNT_LOW, MIDL, and HIGH

    if (ret == ESP_OK) {
        stepCount = (data[2] << 16) | (data[1] << 8) | data[0];  // Combine the bytes into a 24-bit value
        ESP_LOGI(TAG, "Step Count: %d", (int)stepCount);
    } else {
        ESP_LOGE(TAG, "Failed to read step count");
    }
    return ret;
}

esp_err_t MotionDriver::resetStepCount() {
    uint8_t data = 0x0F;  // CTRL_CMD_RESET_PEDOMETER
    return writeRegister(0x11, &data, 1);  // Write to CTRL9 to reset the step count
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
