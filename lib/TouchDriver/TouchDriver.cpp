#include "TouchDriver.h"

#define I2C_SCL GPIO_NUM_7
#define I2C_SDA GPIO_NUM_6
#define I2C_NUM I2C_NUM_0
#define RST_GPIO GPIO_NUM_13
#define INT_GPIO GPIO_NUM_5

static const char *TAG = "TouchDriver";

TouchDriver::TouchDriver() {}

TouchDriver::~TouchDriver() {}

esp_err_t TouchDriver::init() {
    esp_err_t ret;

    // Initialize I2C
    ret = i2cMasterInit();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed");
        return ret;
    }

    ESP_LOGI(TAG, "Touch driver initialized successfully");
    return ESP_OK;
}

esp_err_t TouchDriver::i2cMasterInit() {
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_SDA;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_SCL;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400000;
    conf.clk_flags = 0;

    esp_err_t ret = i2c_param_config(I2C_NUM, &conf);
    if (ret != ESP_OK) {
        return ret;
    }

    return i2c_driver_install(I2C_NUM, conf.mode, 0, 0, 0);
}

esp_err_t TouchDriver::readRegister(uint8_t reg, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t TouchDriver::writeRegister(uint8_t reg, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

TouchData TouchDriver::getTouchCoordinates() {
    uint8_t data[6];
    esp_err_t ret = readRegister(0x01, data, 6);
    if (ret == ESP_OK) {
        TouchData touchData;
        touchData.touch_detected = (data[1] == 0x01);
        touchData.gesture_id = data[0];
        touchData.x = ((data[2] & 0x0F) << 8) | data[3]; // Masking and combining X coordinates
        touchData.y = ((data[4] & 0x0F) << 8) | data[5]; // Masking and combining Y coordinates
        return touchData;
    } else {
        ESP_LOGE(TAG, "Failed to read touch data");
    }
    return TouchData();
}

void TouchDriver::lvglRead(lv_indev_t *drv, lv_indev_data_t *data) {
    TouchData touch = getTouchCoordinates();
    if (touch.touch_detected) {
        data->point.x = touch.x;
        data->point.y = touch.y;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}
