/**
 * @file      LV_Helper.cpp
 * @license   MIT
 *
 * T-Watch S3 LVGL display and touch adapter.
 */

#include "LilyGoLib.h"
#include <lvgl.h>

static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;
static lv_disp_draw_buf_t draw_buf;

void updateLvglHelper()
{
    lv_disp_drv_update(lv_disp_get_default(), &disp_drv);
}

#if LV_VERSION_CHECK(9, 0, 0)
void disp_flush(lv_disp_t *disp, const lv_area_t *area, lv_color_t *color_p)
#else
void disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
#endif
{
    const uint32_t width = area->x2 - area->x1 + 1;
    const uint32_t height = area->y2 - area->y1 + 1;

    watch.startWrite();
    watch.setAddrWindow(area->x1, area->y1, width, height);
#if LV_VERSION_CHECK(9, 0, 0)
    watch.pushColors(reinterpret_cast<uint16_t *>(color_p), width * height);
#else
    watch.pushColors(reinterpret_cast<uint16_t *>(&color_p->full),
                     width * height, true);
#endif
    watch.endWrite();
    lv_disp_flush_ready(disp);
}

#if LV_VERSION_CHECK(9, 0, 0)
void touchpad_read(lv_indev_t *, lv_indev_data_t *data)
#else
void touchpad_read(lv_indev_drv_t *, lv_indev_data_t *data)
#endif
{
    int16_t x = 0;
    int16_t y = 0;
    if (!watch.getPoint(&x, &y))
    {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    switch (watch.getRotation())
    {
    case 0:
        data->point.x = TFT_WIDTH - x;
        data->point.y = TFT_HEIGHT - y;
        break;
    case 1:
        data->point.x = TFT_WIDTH - y;
        data->point.y = x;
        break;
    case 3:
        data->point.x = y;
        data->point.y = TFT_HEIGHT - x;
        break;
    case 2:
    default:
        data->point.x = x;
        data->point.y = y;
        break;
    }
    data->state = LV_INDEV_STATE_PR;
}

#ifndef BOARD_HAS_PSRAM
static constexpr size_t lv_buffer_pixel_count = BOARD_TFT_WIDTH * 50u;
static constexpr size_t lv_buffer_bytes =
    lv_buffer_pixel_count * sizeof(lv_color_t);
static lv_color_t buf[lv_buffer_pixel_count];
static lv_color_t buf1[lv_buffer_pixel_count];
#else
static constexpr size_t lv_buffer_pixel_count =
    BOARD_TFT_WIDTH * BOARD_TFT_HEIHT;
static constexpr size_t lv_buffer_bytes =
    lv_buffer_pixel_count * sizeof(lv_color_t);
static lv_color_t *buf = nullptr;
static lv_color_t *buf1 = nullptr;
#endif

#if LV_USE_LOG
#if LV_VERSION_CHECK(9, 0, 0)
void lv_log_print_g_cb(lv_log_level_t, const char *text)
#else
void lv_log_print_g_cb(const char *text)
#endif
{
    Serial.println(text);
}
#endif

bool beginLvglHelper(bool debug)
{
    lv_init();
#if LV_USE_LOG
    if (debug)
        lv_log_register_print_cb(lv_log_print_g_cb);
#else
    (void)debug;
#endif

#ifdef BOARD_HAS_PSRAM
    buf = static_cast<lv_color_t *>(ps_malloc(lv_buffer_bytes));
    buf1 = static_cast<lv_color_t *>(ps_malloc(lv_buffer_bytes));
    if (!buf || !buf1)
    {
        Serial.printf("LVGL buffer allocation failed (%u bytes each)\n",
                      static_cast<unsigned>(lv_buffer_bytes));
        free(buf);
        free(buf1);
        buf = nullptr;
        buf1 = nullptr;
        return false;
    }
#endif

#if LV_VERSION_CHECK(9, 0, 0)
    lv_disp_t *display = lv_disp_create(BOARD_TFT_WIDTH, BOARD_TFT_HEIHT);
#ifdef BOARD_HAS_PSRAM
    lv_disp_set_draw_buffers(display, buf, buf1, lv_buffer_bytes,
                             LV_DISP_RENDER_MODE_PARTIAL);
#else
    lv_disp_set_draw_buffers(display, buf, nullptr, lv_buffer_bytes,
                             LV_DISP_RENDER_MODE_PARTIAL);
#endif
    lv_disp_set_res(display, BOARD_TFT_WIDTH, BOARD_TFT_HEIHT);
    lv_disp_set_physical_res(display, BOARD_TFT_WIDTH, BOARD_TFT_HEIHT);
    lv_disp_set_flush_cb(display, disp_flush);

    lv_indev_t *input = lv_indev_create();
    lv_indev_set_read_cb(input, touchpad_read);
    lv_indev_set_disp(input, display);
    lv_indev_set_type(input, LV_INDEV_TYPE_POINTER);
#else
    // LVGL 8 expects a pixel count here, not a byte count.
    lv_disp_draw_buf_init(&draw_buf, buf, buf1, lv_buffer_pixel_count);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = BOARD_TFT_WIDTH;
    disp_drv.ver_res = BOARD_TFT_HEIHT;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);
#endif
    return true;
}
