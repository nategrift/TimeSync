/**
 ******************************************************************************
 *							USEFUL ELECTRONICS
 ******************************************************************************/
/**
 ******************************************************************************
 * @file    :  gc9a01.h
 * @author  :  WARD ALMASARANI
 * @version :  v.1.0
 * @date    :  Jan 31, 2023
 * @link    :  https://www.youtube.com/@usefulelectronics
 *			   Hold Ctrl button and click on the link to be directed to
			   Useful Electronics YouTube channel	
 ******************************************************************************/

#ifndef MAIN_GC9A01_H_
#define MAIN_GC9A01_H_


/* INCLUDES ------------------------------------------------------------------*/
#include "esp_lcd_gc9a01.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "esp_err.h"
#include "esp_log.h"

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



/* ENUMORATIONS --------------------------------------------------------------*/

/* STRUCTURES & TYPEDEFS -----------------------------------------------------*/

/* VARIABLES -----------------------------------------------------------------*/
extern esp_lcd_panel_handle_t panel_handle;
/* FUNCTIONS DECLARATION -----------------------------------------------------*/
void gc9a01_displayInit(void);


#endif /* MAIN_GC9A01_H_ */

/*************************************** USEFUL ELECTRONICS*****END OF FILE****/