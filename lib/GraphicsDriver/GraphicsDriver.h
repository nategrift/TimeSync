#ifndef GRAPHICS_DRIVER_H
#define GRAPHICS_DRIVER_H

#include "IUIComponent.h"
#include <string>
#include <map>
#include <memory>
#include <vector>
#include "lvgl.h"

extern "C" {
#include "display.h"
#include "gc9a01.h"
}


// Pin configuration
#define LCD_BL_GPIO GPIO_NUM_2
// #define TP_INT_GPIO GPIO_NUM_5
// #define TP_SDA_GPIO GPIO_NUM_6
// #define TP_SCL_GPIO GPIO_NUM_7
// #define LCD_DC_GPIO GPIO_NUM_8
// #define LCD_CS_GPIO GPIO_NUM_9
// #define LCD_CLK_GPIO GPIO_NUM_10
// #define LCD_MOSI_GPIO GPIO_NUM_11
// #define LCD_MISO_GPIO GPIO_NUM_12
// #define TP_RST_GPIO GPIO_NUM_13
// #define LCD_RST_GPIO GPIO_NUM_14

#define LCD_HOST  SPI2_HOST
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (20 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_SCLK           GPIO_NUM_10
#define EXAMPLE_PIN_NUM_MOSI           GPIO_NUM_11
#define EXAMPLE_PIN_NUM_MISO           GPIO_NUM_12
#define EXAMPLE_PIN_NUM_LCD_DC         GPIO_NUM_8
#define EXAMPLE_PIN_NUM_LCD_RST        GPIO_NUM_14
#define EXAMPLE_PIN_NUM_LCD_CS         GPIO_NUM_9
#define EXAMPLE_PIN_NUM_BK_LIGHT       GPIO_NUM_2

#define EXAMPLE_LCD_H_RES              240
#define EXAMPLE_LCD_V_RES              240

#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8


class GraphicsDriver {
private:
    static void lvgl_task(void *arg);

public:
    GraphicsDriver();
    void init();
    void addTextToCenter(const char *text);
};

#endif // GRAPHICS_DRIVER_H
