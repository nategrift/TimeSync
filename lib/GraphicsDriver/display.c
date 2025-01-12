/**
 ******************************************************************************
 *							USEFUL ELECTRONICS
 ******************************************************************************/
/**
 ******************************************************************************
 * @file    :  display.c
 * @author  :  WARD ALMASARANI
 * @version :  v.1.0
 * @date    :  Feb 1, 2023
 * @link    :  https://www.youtube.com/@usefulelectronics
 *			   Hold Ctrl button and click on the link to be directed to
			   Useful Electronics YouTube channel	
 ******************************************************************************/


/* INCLUDES ------------------------------------------------------------------*/
#include "display.h"
#include "lvgl.h"


/* PRIVATE STRUCTRES ---------------------------------------------------------*/

/* VARIABLES -----------------------------------------------------------------*/
lv_display_t *disp_drv;  // contains callback functions
/* DEFINITIONS ---------------------------------------------------------------*/

/* MACROS --------------------------------------------------------------------*/
static const char *TAG = "display-driver";
/* PRIVATE FUNCTIONS DECLARATION ---------------------------------------------*/
// extern void example_lvgl_demo_ui(lv_disp_t *disp);
/* FUNCTION PROTOTYPES -------------------------------------------------------*/


static esp_timer_handle_t lvgl_tick_timer = NULL;

bool display_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    assert(disp_drv);
    lv_display_flush_ready(disp_drv);
    return false;
}

static void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, (void*)px_map);

}

/* Rotate display and touch, when rotated screen in LVGL. Called when driver parameters are updated. */
static void example_lvgl_port_update_callback(lv_display_t *drv)
{
    ESP_LOGI(TAG, "example_lvgl_port_update_callback");
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) lv_display_get_user_data(drv);

    switch (lv_display_get_rotation(drv)) {
    case LV_DISPLAY_ROTATION_0:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, true, false);
        break;

    case LV_DISPLAY_ROTATION_90:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, true, true);

        break;

    case LV_DISPLAY_ROTATION_180:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, false, true);
        break;

    case LV_DISPLAY_ROTATION_270:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, false, false);
        break;
    }
}



static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static void lvgl_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_display_t *disp = lv_event_get_target(e);

    if (code == LV_EVENT_RESOLUTION_CHANGED) {
        example_lvgl_port_update_callback(disp);
    }
}

void lvglDisplayConfig(void)
{
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    disp_drv = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);

    // static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)

    // alloc draw buffers used by LVGL
    // allocating 1/4 buffer in PSRAM
    #define BYTES_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))
    int buff_size = EXAMPLE_LCD_H_RES * (EXAMPLE_LCD_V_RES/4) * BYTES_PER_PIXEL;
    uint8_t *buf1 = heap_caps_malloc(buff_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    assert(buf1);
    uint8_t *buf2 = heap_caps_malloc(buff_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    assert(buf2);
    lv_display_set_buffers(disp_drv, buf1, buf2, buff_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // ESP_LOGI(TAG, "Register display driver to LVGL");
    // (&disp_drv);
    // disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    // disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    // disp_drv.flush_cb = example_lvgl_flush_cb;
    // disp_drv.drv_update_cb = example_lvgl_port_update_callback;
    // disp_drv.draw_buf = &disp_buf;
    // disp_drv.user_data = panel_handle;
    // lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    lv_display_set_user_data(disp_drv, panel_handle);
    
    lv_display_set_flush_cb(disp_drv, example_lvgl_flush_cb);

    // Register the event handler for size change
    lv_display_add_event_cb(disp_drv, lvgl_event_handler, LV_EVENT_RESOLUTION_CHANGED, NULL);

    ESP_LOGI(TAG, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));
}


void pause_lvgl_tick_timer() {
    if (lvgl_tick_timer != NULL) {
        ESP_LOGI(TAG, "Pausing LVGL tick timer");
        ESP_ERROR_CHECK(esp_timer_stop(lvgl_tick_timer));
    }
}

void resume_lvgl_tick_timer() {
    if (lvgl_tick_timer != NULL) {
        ESP_LOGI(TAG, "Resuming LVGL tick timer");
        ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));
    }
}