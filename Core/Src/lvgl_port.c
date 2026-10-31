#include "lvgl_port.h"

#include "cmsis_os.h"
#include "lcd8080.h"
#include "touch_ft6336.h"
#include "lvgl.h"
#include "demos/lv_demos.h"

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
    uint32_t row;
    uint32_t row_stride;
    uint32_t row_bytes;
    lv_draw_buf_t *draw_buf;

    if ((area == NULL) || (px_map == NULL)) {
        lv_display_flush_ready(display);
        return;
    }
    if ((area->x1 < 0) || (area->y1 < 0) ||
        (area->x2 >= (int32_t)LCD8080_WIDTH) ||
        (area->y2 >= (int32_t)LCD8080_HEIGHT) ||
        (area->x1 > area->x2) || (area->y1 > area->y2)) {
        /* 防止异常刷新区域把 LCD 地址窗口写到有效范围之外。 */
        lv_display_flush_ready(display);
        return;
    }
    width = (uint32_t)(area->x2 - area->x1 + 1);
    height = (uint32_t)(area->y2 - area->y1 + 1);
    LCD8080_SetWindow((uint16_t)area->x1, (uint16_t)area->y1,
                      (uint16_t)area->x2, (uint16_t)area->y2);

    /*
     * LVGL 的局部绘图缓冲按“每行 stride”组织，px_map 只保证指向当前
     * 区域的首行。stride 可能大于有效像素宽度（例如 4 字节对齐），
     * 不能把整个缓冲直接当作 width*height 个连续像素发送，否则行尾
     * 填充字节会变成屏幕上的白条/错位数据。
     */
    row_bytes = width * sizeof(uint16_t);
    draw_buf = lv_display_get_buf_active(display);
    row_stride = (draw_buf != NULL) ? draw_buf->header.stride : row_bytes;
    if (row_stride < row_bytes) {
        row_stride = row_bytes;
    }
    for (row = 0U; row < height; ++row) {
        const uint16_t *line = (const uint16_t *)(px_map + row * row_stride);
        if (row == 0U) {
            LCD8080_WritePixels(line, width);
        }
        else {
            LCD8080_WritePixelsContinue(line, width);
        }
    }
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
    /* 即使复位后的识别读取失败，也继续轮询；有些 FT6336 模块的设备
     * 模式寄存器读回不稳定，但坐标寄存器仍可正常工作。 */
    if ((FT6336_ReadPoint(&point) == HAL_OK)) {
        read_errors = 0U;
        g_ft6336_status = HAL_OK;
        lvgl_touch_ready = 1U;
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

/* 之前的纯色背景和 touch 按钮暂时保留，测试 widgets demo 时不执行。 */
#if 0
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
#endif

void LVGL_Port_Init(void)
{
    /* 触摸复位需要约 320 ms；在 LVGL 所属任务中等待不会阻塞系统启动。 */
    g_ft6336_status = FT6336_Init();
    lvgl_touch_ready = (g_ft6336_status == HAL_OK) ? 1U : 0U;
    lv_init();
    lv_tick_set_cb(lvgl_tick_get);

    lvgl_display = lv_display_create(LCD8080_WIDTH, LCD8080_HEIGHT);
    lv_display_set_default(lvgl_display);
    lv_display_set_color_format(lvgl_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(lvgl_display, lvgl_flush_cb);
    lv_display_set_buffers(lvgl_display, lvgl_draw_buffer, NULL,
                           sizeof(lvgl_draw_buffer),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lvgl_touch = lv_indev_create();
    lv_indev_set_type(lvgl_touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(lvgl_touch, lvgl_display);
    lv_indev_set_read_cb(lvgl_touch, lvgl_touch_read_cb);
    /* 输入由 LVGL 任务显式轮询，避免 widgets demo 的动画定时器延迟触摸。
     * 暂停输入设备自己的定时器，防止同一触摸在一个周期内被读两次。 */
    lv_timer_pause(lv_indev_get_read_timer(lvgl_touch));

    /* 官方 widgets demo：包含 Profile、Analytics、Shop 等多个页面。 */
    lv_demo_widgets();
}

void LVGL_Port_Task(void *argument)
{
    static uint32_t next_touch_poll;

    (void)argument;
    LVGL_Port_Init();
    next_touch_poll = HAL_GetTick();
    for (;;) {
        /* FT6336 只在初始化时复位；运行时 I2C 短暂异常不重置芯片，
         * 避免页面切换期间出现数百毫秒的假死。读取会自动恢复。 */
        if ((int32_t)(HAL_GetTick() - next_touch_poll) >= 0) {
            next_touch_poll = HAL_GetTick() + 10U;
            if (lvgl_touch != NULL) {
                lv_indev_read(lvgl_touch);
            }
        }
        (void)lv_timer_handler();
        osDelay(5U);
    }
}
