#include "MotionDriver.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "ConfigManager.h"

#define I2C_SCL GPIO_NUM_7
#define I2C_SDA GPIO_NUM_6
#define I2C_NUM I2C_NUM_0

static const char *TAG = "MotionDriver";

MotionDriver::MotionDriver() {}

MotionDriver::~MotionDriver() {}

esp_err_t MotionDriver::init() {
    esp_err_t ret;

    // Verify chip ID
    uint8_t chip_id;
    ret = readRegister(0x00, &chip_id, 1);  // 0x00 is WHO_AM_I register
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read chip ID");
        return ret;
    }
    if (chip_id != 0x05) {
        ESP_LOGE(TAG, "Wrong chip ID: 0x%02x", chip_id);
        return ESP_ERR_INVALID_VERSION;
    }

    // Reset the device
    uint8_t reset_cmd = 0xB0;
    ret = writeRegister(0x60, &reset_cmd, 1);  // 0x60 is RESET register
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset device");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(50));

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

    uint8_t accelConfig = 0b00010111;  // CTRL2: test = 0,aFS = 001 (±4g), aODR = 0110 (125 Hz)
    ret = writeRegister(0x03, &accelConfig, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure accelerometer");
        return ret;
    }

    uint8_t gyroConfig = 0b00100110;  // CTRL3: test = 0, 010 - ±64 dps, gODR = 0110 = 112.1 ODR Rate (Hz)
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
    uint8_t x_low;
    uint8_t x_high;
    esp_err_t ret = readRegister(0x3B, &x_low, 1);
    ret = readRegister(0x3C, &x_high, 1);

    uint8_t y_low;
    uint8_t y_high;
    ret = readRegister(0x3D, &y_low, 1);
    ret = readRegister(0x3E, &y_high, 1);

    uint8_t z_high;
    uint8_t z_low;
    ret = readRegister(0x3F, &z_low, 1);
    ret = readRegister(0x40, &z_high, 1);
    
    float scale_factor = 64.0f / 32768.0f;
    if (ret == ESP_OK) {
        int16_t raw_x = (x_high << 8) | x_low;
        int16_t raw_y = (y_high << 8) | y_low;
        int16_t raw_z = (z_high << 8) | z_low;
        x = (float)raw_x * scale_factor;
        y = (float)raw_y * scale_factor;
        z = (float)raw_z * scale_factor;
    } else {
        ESP_LOGE(TAG, "Failed to read gyroscope data");
    }
    return ret;
}

esp_err_t MotionDriver::readAccelerometer(float &x, float &y, float &z) {
    uint8_t x_low;
    uint8_t x_high;
    esp_err_t ret = readRegister(0x35, &x_low, 1);
    ret = readRegister(0x36, &x_high, 1);

    uint8_t y_low;
    uint8_t y_high;
    ret = readRegister(0x37, &y_low, 1);
    ret = readRegister(0x38, &y_high, 1);

    uint8_t z_high;
    uint8_t z_low;
    ret = readRegister(0x39, &z_low, 1);
    ret = readRegister(0x3A, &z_high, 1);
    
    float scale_factor = 4.0f / 32768.0f;
    if (ret == ESP_OK) {
        int16_t raw_x = (x_high << 8) | x_low;
        int16_t raw_y = (y_high << 8) | y_low;
        int16_t raw_z = (z_high << 8) | z_low;
        x = (float)raw_x * scale_factor;
        y = (float)raw_y * scale_factor;
        z = (float)raw_z * scale_factor;
    } else {
        ESP_LOGE(TAG, "Failed to read gyroscope data");
    }
    return ret;
}


// REGISTERS    
// CAL1_L 11 -> 0B
// CAL1_H 12 -> 0C
// CAL2_L 13 -> 0D
// CAL2_H 14 -> 0E
// CAL3_L 15 -> 0F
// CAL3_H 16 -> 10
// CAL4_L 17 -> 11
// CAL4_H 18 -> 12
esp_err_t MotionDriver::configurePedometer() {
    uint16_t pedSampleCnt = 50; // sample
    uint16_t pedFixPeak2peak = 200; //mg
    uint16_t pedFixPeak = 100; //mg
    uint16_t pedTimeUp = 100; // samples
    uint8_t pedTimeLow = 12; // samples
    uint8_t pedCntEntry = 4; // samples
    uint8_t pedFixPrecision = 0; //mg
    uint8_t pedSigCount = 4; //mg

    int level = ConfigManager::getConfigInt("General", "PedometerLevel");
    if (level == 1) {
        pedFixPeak2peak = 200; // higher for more strict threshold
        pedFixPeak = 100; // higher for more strict threshold
        pedTimeUp = 150; // lower for more strict threshold
        pedTimeLow = 12; // higher for more strict threshold
    } else if (level == 2) {
        pedFixPeak2peak = 250; 
        pedFixPeak = 125; 
        pedTimeUp = 125;
        pedTimeLow = 16; 
    } else if (level == 3) {
        pedFixPeak2peak = 300; 
        pedFixPeak = 150; 
        pedTimeUp = 100;
        pedTimeLow = 20; 
    } else if (level == 4) {
        pedFixPeak2peak = 350; 
        pedFixPeak = 175; 
        pedTimeUp = 80;
        pedTimeLow = 24; 
    }  else if (level == 5) {
        pedFixPeak2peak = 400; 
        pedFixPeak = 200; 
        pedTimeUp = 60;
        pedTimeLow = 28; 
    }

    // First set of pedometer parameters
    uint8_t cal4H_1 = 0x01;  // Mentioned we are writing first params
    writeRegister(0x12, &cal4H_1, 1);  // CAL4_H

    // CAL1 register (0x0B, 0x0C)
    uint8_t pedSampleCnt_L = pedSampleCnt & 0xFF;
    uint8_t pedSampleCnt_H = (pedSampleCnt >> 8) & 0xFF;
    writeRegister(0x0B, &pedSampleCnt_L, 1);  // CAL1_L
    writeRegister(0x0C, &pedSampleCnt_H, 1);  // CAL1_H

    // CAL2 register (0x0D, 0x0E)
    uint8_t pedFixPeak2Peak_L = pedFixPeak2peak & 0xFF;
    uint8_t pedFixPeak2Peak_H = (pedFixPeak2peak >> 8) & 0xFF;
    writeRegister(0x0D, &pedFixPeak2Peak_L, 1);  // CAL2_L
    writeRegister(0x0E, &pedFixPeak2Peak_H, 1);  // CAL2_H

    // CAL3 register (0x0F, 0x10)
    uint8_t pedFixPeak_L = pedFixPeak & 0xFF;
    uint8_t pedFixPeak_H = (pedFixPeak >> 8) & 0xFF;
    writeRegister(0x0F, &pedFixPeak_L, 1);  // CAL3_L
    writeRegister(0x10, &pedFixPeak_H, 1);  // CAL3_H

    // Trigger first configuration CTRL_9 Function calling
    uint8_t configCommand1 = 0x0D;
    writeRegister(0x0A, &configCommand1, 1);

    vTaskDelay(pdMS_TO_TICKS(500));

    // Second set of pedometer parameters
    // CAL4 register (0x11, 0x12)
    uint8_t cal4H_2 = 0x02;  // Second command
    writeRegister(0x12, &cal4H_2, 1);  // CAL4_H

    // CAL1 register (0x0B, 0x0C)
    uint8_t pedTimeUp_L = pedTimeUp & 0xFF;
    uint8_t pedTimeUp_H = (pedTimeUp >> 8) & 0xFF;
    writeRegister(0x0B, &pedTimeUp_L, 1);
    writeRegister(0x0C, &pedTimeUp_H, 1);

    // CAL2 register (0x0D, 0x0E)
    writeRegister(0x0D, &pedTimeLow, 1);
    writeRegister(0x0E, &pedCntEntry, 1);

    // CAL3 register (0x0F, 0x10)
    writeRegister(0x0F, &pedFixPrecision, 1);
    writeRegister(0x10, &pedSigCount, 1);

    // Trigger second configuration
    uint8_t configCommand2 = 0x0D;
    writeRegister(0x0A, &configCommand2, 1);

    return ESP_OK;
}

esp_err_t MotionDriver::enablePedometer() {
    // Sleep for 20ms to allow the pedometer configuration to settle
    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t data = 0b00010000;  // Enable pedometer (CTRL8.bit4 = 1)
    return writeRegister(0x09, &data, 1);  // Write to CTRL8 to enable pedometer
}

esp_err_t MotionDriver::isPedometerRunning(bool &isRunning) {
    uint8_t data;
    esp_err_t ret = readRegister(0x09, &data, 1);  // Read CTRL8 register
    if (ret == ESP_OK) {
        uint8_t enabledPedometer = 0b00010000;  // Enable pedometer (CTRL8.bit4 = 1)
        isRunning = (data & enabledPedometer) ? true : false;  // Check if Pedo_EN (bit 4) is set
        ESP_LOGI(TAG, "Pedometer is %s", isRunning ? "running" : "stopped");
    } else {
        ESP_LOGE(TAG, "Failed to read pedometer status");
    }
    return ret;
}

esp_err_t MotionDriver::readStepCount(uint32_t &stepCount) {
    uint8_t low;
    uint8_t mid;
    uint8_t high;
    esp_err_t ret = readRegister(0x5A, &low, 1); 
    ret = readRegister(0x5B, &mid, 1); 
    ret = readRegister(0x5C, &high, 1); 

    if (ret == ESP_OK) {
        stepCount = (high << 16) | (mid << 8) | low;  
        ESP_LOGI(TAG, "Step Count: %lu", stepCount);
    } else {
        ESP_LOGE(TAG, "Failed to read step count");
    }
    return ret;
}

esp_err_t MotionDriver::resetStepCount() {
    uint8_t data = 0x0F;  // CTRL_CMD_RESET_PEDOMETER
    return writeRegister(0x0A, &data, 1);  // Write to CTRL9 to reset the step count
}


esp_err_t MotionDriver::readRegister(uint8_t reg, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (QMI8658_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (QMI8658_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);  // Read all but last byte with ACK
    }
    i2c_master_read(cmd, data + len - 1, 1, I2C_MASTER_NACK); // Read last byte with NACK
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
