// TouchDriver.cpp
#include "TouchDriver.h"
#include "esp_log.h"

static const char *TAG = "TOUCH_SETUP";

static SemaphoreHandle_t touch_mux;
static esp_lcd_touch_handle_t tp = NULL;

TouchDriver::TouchDriver() {
}

TouchDriver::~TouchDriver() {
    if (touch_mux != nullptr) {
        vSemaphoreDelete(touch_mux);
    }
}

void TouchDriver::init() {
    esp_lcd_panel_io_handle_t tp_io_handle = nullptr;

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
    };

    i2c_conf.master.clk_speed = 400000;

    ESP_LOGI(TAG, "Initializing I2C for display touch");
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM, i2c_conf.mode, 0, 0, 0));

    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
    ESP_LOGI(TAG, "esp_lcd_new_panel_io_i2c");
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c_v1(I2C_NUM, &tp_io_config, &tp_io_handle));

    // Create the semaphore
    touch_mux = xSemaphoreCreateBinary();
    if (touch_mux == nullptr) {
        ESP_LOGE(TAG, "Failed to create touch mutex");
        return;
    }

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = 240,
        .y_max = 240,
        .rst_gpio_num = RST_GPIO,
        .int_gpio_num = INT_GPIO,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 1,
            .mirror_x = 0,
            .mirror_y = 1,
        },
        .interrupt_callback = touchCallback,
    };

    ESP_LOGI(TAG, "esp_lcd_touch_new_i2c_cst816s");
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst816s(tp_io_handle, &tp_cfg, &tp));
}

void TouchDriver::initTouchForGraphics() {
    if (tp != nullptr) {
        lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = touchpadRead;
        indev_drv.user_data = tp;
        lv_indev_drv_register(&indev_drv);
    } else {
        ESP_LOGE(TAG, "Failed to init touch for graphics, touch_handle was false. Call init() fist.");
    }

}

void TouchDriver::touchpadRead(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    uint8_t touchpad_cnt = 0;

    if (xSemaphoreTake(touch_mux, 0) == pdTRUE) {
        esp_lcd_touch_read_data((esp_lcd_touch_handle_t)drv->user_data);
        xSemaphoreGive(touch_mux);
    }

    bool touchpad_pressed = esp_lcd_touch_get_coordinates((esp_lcd_touch_handle_t)drv->user_data, touchpad_x, touchpad_y, nullptr, &touchpad_cnt, 1);

    if (touchpad_pressed && touchpad_cnt > 0) {
        data->point.x = touchpad_x[0];
        data->point.y = touchpad_y[0];
        data->state = LV_INDEV_STATE_PRESSED;
        printf("data->point.x = %u \n", data->point.x);
        printf("data->point.y = %u \n", data->point.y);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void TouchDriver::touchCallback(esp_lcd_touch_handle_t tp) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(touch_mux, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}
