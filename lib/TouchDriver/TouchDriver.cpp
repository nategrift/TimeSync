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

    // Reset the touch chip
    ret = reset();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Reset failed");
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

esp_err_t TouchDriver::reset() {
    gpio_set_direction(RST_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(RST_GPIO, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(RST_GPIO, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    return ESP_OK;
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

void TouchDriver::readTouchData() {
    uint8_t data[6];
    esp_err_t ret = readRegister(0x01, data, 6);
    if (ret == ESP_OK) {
        if (data[1] == 0x01) { // Check if touch is detected
            printTouchCoordinates(data);
        }
    } else {
        ESP_LOGE(TAG, "Failed to read touch data");
    }
}

void TouchDriver::printTouchCoordinates(const uint8_t *data) {
    // Assuming data format from the datasheet: 
    // Byte 0: Gesture ID
    // Byte 1: Event Flag
    // Byte 2: X High Byte
    // Byte 3: X Low Byte
    // Byte 4: Y High Byte
    // Byte 5: Y Low Byte

    uint16_t x = ((data[2] & 0x0F) << 8) | data[3]; // Masking and combining X coordinates
    uint16_t y = ((data[4] & 0x0F) << 8) | data[5]; // Masking and combining Y coordinates

    // Print coordinates
    // ESP_LOGI(TAG, "Touch at X: %d, Y: %d", x, y);

    // Print gesture ID
    // ESP_LOGI(TAG, "Gesture ID: 0x%02x", data[0]);
}

bool TouchDriver::getTouchCoordinates(uint16_t &x, uint16_t &y) {
    uint8_t data[6];
    esp_err_t ret = readRegister(0x01, data, 6);
    if (ret == ESP_OK && data[1] == 0x01) { // Check if touch is detected
        x = ((data[2] & 0x0F) << 8) | data[3]; // Masking and combining X coordinates
        y = ((data[4] & 0x0F) << 8) | data[5]; // Masking and combining Y coordinates
        return true;
    }
    return false;
}

void TouchDriver::lvglRead(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    uint16_t x, y;
    
    bool touched = getTouchCoordinates(x, y);
    if (touched) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}
