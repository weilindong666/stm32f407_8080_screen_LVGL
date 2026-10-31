#ifndef TOUCH_FT6336_H
#define TOUCH_FT6336_H

#include "main.h"

/* 默认按 I2C2 接线：SCL=PF1，SDA=PB11；RST=PB15，INT=PB2。
 * 若实际接在 I2C1（SCL=PB6，SDA=PB7），将这里改为 hi2c1。
 * 本阶段轮询触摸寄存器，不依赖 PB2 中断；I2C 总线需有上拉电阻。 */
#define FT6336_I2C_HANDLE        hi2c2

#ifdef __cplusplus
extern "C" {
#endif

/* FT6336 的 7 位 I2C 地址；HAL 使用左移一位后的地址。 */
#define FT6336_I2C_ADDRESS       (0x38U << 1)
/* 单次 I2C 读取超时（毫秒）。触摸在 LVGL 任务中轮询，超时过长会让
 * 页面切换看起来像死机；总线异常时让本次采样快速失败即可。 */
#define FT6336_I2C_TIMEOUT_MS    10U

/* 原始触摸坐标通常以屏幕竖向 320×480 为基准。 */
#define FT6336_RAW_WIDTH         320U
#define FT6336_RAW_HEIGHT        480U

/* 触摸安装方向校正：先交换轴，再反转；均为 0=关闭，1=开启。
 * 当前横屏映射：x=raw_y，y=319-raw_x。
 * 根据实物反馈取消左右反转，保留上下方向校正。
 * LCD 的 MIRROR_X/Y 会在这之后自动叠加，不要在这里重复补偿。
 * LVGL 使用转换后的有效坐标判断按钮命中，越界坐标不会触发按钮。 */
#include "lcd8080.h"
#define FT6336_SWAP_XY           LCD8080_LANDSCAPE
#define FT6336_INVERT_X          0U /* 0=不反转左右坐标，1=反转 */
#define FT6336_INVERT_Y          1U /* 0=不反转上下坐标，1=反转 */

typedef struct
{
    uint16_t raw_x;
    uint16_t raw_y;
    uint16_t x;
    uint16_t y;
    uint8_t  event;
    uint8_t  pressed;          /* 1=按下/接触，0=松手 */
    uint8_t  coordinates_valid; /* 1=坐标在原始量程内 */
} FT6336_PointTypeDef;

/* 可在调试器观察：HAL_OK=最近通信成功；按下计数每次新按下加一。
 * latest_point 同时保留原始坐标和转换后的坐标。 */
extern volatile HAL_StatusTypeDef g_ft6336_status;
extern volatile FT6336_PointTypeDef g_ft6336_latest_point;
extern volatile uint32_t g_ft6336_press_count;

/* 需要在 FreeRTOS 任务中调用；复位等待使用 osDelay。 */
HAL_StatusTypeDef FT6336_Init(void);
HAL_StatusTypeDef FT6336_ReadPoint(FT6336_PointTypeDef *point);
void FT6336_TransformPoint(uint16_t raw_x, uint16_t raw_y,
                           uint16_t *x, uint16_t *y);

#ifdef __cplusplus
}
#endif

#endif /* TOUCH_FT6336_H */
