/**
 * @file lv_conf.h
 * Configuration file for v8.0.0
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
 * Graphical settings
 *====================*/

/* Maximal horizontal and vertical resolution to support by the library.*/
#define LV_HOR_RES_MAX          (240)
#define LV_VER_RES_MAX          (240)

/* Color depth:
 * - 1:  1 byte per pixel
 * - 8:  RGB233
 * - 16: RGB565
 * - 32: ARGB8888
 */
#define LV_COLOR_DEPTH     16
#define LV_COLOR_16_SWAP 1

/*================
 *  Memory manager settings
 *================*/

/* Size of the memory used by `lv_mem_alloc`. */
#define LV_MEM_SIZE    (32U * 1024U)

/*================
 * Input device settings
 *================*/

/* 1: Enable input devices */
#define LV_USE_INDEV    1


#endif /*LV_CONF_H*/
