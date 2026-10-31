#ifndef LCD8080_H
#define LCD8080_H

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 屏幕方向参数，三个参数可以自由组合：
 *
 * LCD8080_LANDSCAPE：0 = 竖向（320×480），1 = 横向（480×320）
 * LCD8080_MIRROR_X ：0 = 左右正常，1 = 左右翻转
 * LCD8080_MIRROR_Y ：0 = 上下正常，1 = 上下翻转
 *
 * LVGL 分辨率和触摸坐标转换会跟随这些参数；修改后重新编译生效。
 */
#define LCD8080_LANDSCAPE 1U
#define LCD8080_MIRROR_X  0U
#define LCD8080_MIRROR_Y  0U

#if LCD8080_LANDSCAPE
#define LCD8080_WIDTH   480U
#define LCD8080_HEIGHT  320U
#else
#define LCD8080_WIDTH   320U
#define LCD8080_HEIGHT  480U
#endif

#define LCD8080_RED     0xF800U
#define LCD8080_BLACK   0x0000U
#define LCD8080_WHITE   0xFFFFU
#define LCD8080_BLUE    0x001FU
#define LCD8080_GREEN   0x07E0U
#define LCD8080_YELLOW  0xFFE0U
#define LCD8080_CYAN    0x07FFU
#define LCD8080_MAGENTA 0xF81FU

void LCD8080_Init(void);
void LCD8080_DisplayOn(void);
void LCD8080_DisplayOff(void);
void LCD8080_SetWindow(uint16_t x0, uint16_t y0,
                       uint16_t x1, uint16_t y1);
void LCD8080_Fill(uint16_t color);
void LCD8080_FillWindow(uint16_t x0, uint16_t y0,
                        uint16_t x1, uint16_t y1, uint16_t color);
/* 先 SetWindow，再调用本函数：发送 0x2C，按行写入连续 RGB565 像素。
 * pixels 使用本机半字顺序，无需交换高低字节；count 不能超过窗口面积。 */
void LCD8080_WritePixels(const uint16_t *pixels, uint32_t count);
/* 在同一个窗口内继续发送像素，不重新发送 0x2C；用于 LVGL 按行
 * 处理带 stride 的局部缓冲。 */
void LCD8080_WritePixelsContinue(const uint16_t *pixels, uint32_t count);
void LCD8080_DrawOrientationLabel(void);

#ifdef __cplusplus
}
#endif

#endif /* LCD8080_H */
