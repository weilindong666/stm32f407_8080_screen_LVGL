#include "lvgl_port.h"

#include "cmsis_os.h"
#include "lcd8080.h"
#include "touch_ft6336.h"
#include "lvgl.h"

#define LVGL_DRAW_LINES 20U

static uint16_t lvgl_draw_buffer[LCD8080_WIDTH * LVGL_DRAW_LINES]
    __attribute__((aligned(4)));
static lv_display_t *lvgl_display;
static lv_indev_t *lvgl_touch;
static uint8_t lvgl_touch_ready;

static uint32_t lvgl_tick_get(void)
{
    return HAL_GetTick();
}

static void lvgl_flush_cb(lv_display_t *display, const lv_area_t *area,
                          uint8_t *px_map)
{
    uint32_t width;
    uint32_t height;

    if ((area == NULL) || (px_map == NULL)) {
        lv_display_flush_ready(display);
        return;
    }
    width = (uint32_t)(area->x2 - area->x1 + 1);
    height = (uint32_t)(area->y2 - area->y1 + 1);
    LCD8080_SetWindow((uint16_t)area->x1, (uint16_t)area->y1,
                      (uint16_t)area->x2, (uint16_t)area->y2);
    /* LVGL is configured for native little-endian RGB565, matching a
     * 16-bit FSMC write to the ST7796 GRAM data register. */
    LCD8080_WritePixels((const uint16_t *)px_map, width * height);
    lv_display_flush_ready(display);
}

static void lvgl_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    static uint16_t last_x;
    static uint16_t last_y;
    static uint8_t last_pressed;
    static uint8_t read_errors;
    FT6336_PointTypeDef point;

    (void)indev;
    if (!lvgl_touch_ready) {
        data->state = LV_INDEV_STATE_RELEASED;
        data->point.x = (int32_t)last_x;
        data->point.y = (int32_t)last_y;
        return;
    }
    if ((FT6336_ReadPoint(&point) == HAL_OK)) {
        read_errors = 0U;
        g_ft6336_status = HAL_OK;
        g_ft6336_latest_point = point;
        if (point.pressed && point.coordinates_valid) {
            last_x = point.x;
            last_y = point.y;
            last_pressed = 1U;
        }
        else {
            last_pressed = 0U;
        }
        data->point.x = (int32_t)last_x;
        data->point.y = (int32_t)last_y;
        data->state = last_pressed ? LV_INDEV_STATE_PRESSED :
                                     LV_INDEV_STATE_RELEASED;
    } else {
        /* 短暂 I2C 错误不立即伪造松手，避免一次点击被重复触发。 */
        g_ft6336_status = HAL_ERROR;
        if (read_errors < 3U) {
            ++read_errors;
        } else {
            last_pressed = 0U;
        }
        data->point.x = (int32_t)last_x;
        data->point.y = (int32_t)last_y;
        data->state = last_pressed ? LV_INDEV_STATE_PRESSED :
                                     LV_INDEV_STATE_RELEASED;
    }
}

static void lvgl_button_event_cb(lv_event_t *event)
{
    static const uint32_t colors[] = {
        0xFF0000U, 0x00FF00U, 0x0000FFU, 0xFFFF00U,
        0xFF00FFU, 0x00FFFFU, 0xFFFFFFU, 0x000000U
    };
    static uint32_t color_index;

    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        color_index = (color_index + 1U) %
                      (sizeof(colors) / sizeof(colors[0]));
        lv_obj_set_style_bg_color(lv_screen_active(),
                                  lv_color_hex(colors[color_index]),
                                  LV_PART_MAIN);
    }
}

static void lvgl_create_demo(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_t *button;
    lv_obj_t *label;

    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFF0000U), LV_PART_MAIN);
    button = lv_button_create(screen);
    lv_obj_set_size(button, 180, 80);
    lv_obj_center(button);
    lv_obj_add_event_cb(button, lvgl_button_event_cb, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(button);
    lv_label_set_text(label, "touch");
    lv_obj_center(label);
}

void LVGL_Port_Init(void)
{
    /* 触摸复位需要约 320 ms；在 LVGL 所属任务中等待不会阻塞系统启动。 */
    g_ft6336_status = FT6336_Init();
    lvgl_touch_ready = (g_ft6336_status == HAL_OK) ? 1U : 0U;
    lv_init();
    lv_tick_set_cb(lvgl_tick_get);

    lvgl_display = lv_display_create(LCD8080_WIDTH, LCD8080_HEIGHT);
    lv_display_set_color_format(lvgl_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(lvgl_display, lvgl_flush_cb);
    lv_display_set_buffers(lvgl_display, lvgl_draw_buffer, NULL,
                           sizeof(lvgl_draw_buffer),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lvgl_touch = lv_indev_create();
    lv_indev_set_type(lvgl_touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(lvgl_touch, lvgl_display);
    lv_indev_set_read_cb(lvgl_touch, lvgl_touch_read_cb);

    lvgl_create_demo();
}

void LVGL_Port_Task(void *argument)
{
    static uint32_t next_touch_retry;

    (void)argument;
    LVGL_Port_Init();
    next_touch_retry = HAL_GetTick() + 1000U;
    for (;;) {
        /* 触摸芯片暂时未响应时保留界面，并每秒重试一次初始化。 */
        if (!lvgl_touch_ready &&
            ((int32_t)(HAL_GetTick() - next_touch_retry) >= 0)) {
            next_touch_retry = HAL_GetTick() + 1000U;
            g_ft6336_status = FT6336_Init();
            lvgl_touch_ready = (g_ft6336_status == HAL_OK) ? 1U : 0U;
        }
        (void)lv_timer_handler();
        osDelay(5U);
    }
}
