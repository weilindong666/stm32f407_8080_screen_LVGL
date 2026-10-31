/* LVGL 10 configuration for STM32F407 + one FreeRTOS GUI task.
 * 这个配置只保留当前 RGB565 触摸按钮所需功能；工程内 lvgl 中的完整
 * 源码、examples 和 demos 仍然保留，后续可按需打开。 */
#ifndef LV_CONF_H
#define LV_CONF_H

/* LVGL 只由 lvglTask 调用，避免再创建 LVGL 内部绘图线程。 */
#define LV_USE_OS LV_OS_NONE

/* ST7796 8080 总线使用小端半字写入 RGB565。 */
#define LV_COLOR_FORMAT_DEFAULT LV_COLOR_FORMAT_RGB565
#define LV_DEF_REFR_PERIOD 20
#define LV_DRAW_BUF_STRIDE_ALIGN 1
#define LV_DRAW_BUF_ALIGN 4

/* LVGL 内存池；widgets demo 的分页、图表和动画需要更大的余量。
 * 绘图缓冲另占约 19.2 KB，FreeRTOS 堆和任务栈仍独立分配。 */
#define LV_MEM_SIZE (64U * 1024U)
#define LV_MEM_ADR 0

/* 当前界面使用软件渲染和基础主题；关闭不需要的矢量/日志功能。 */
#define LV_USE_DRAW_SW 1
#define LV_DRAW_SW_COMPLEX 1
#define LV_USE_DRAW_VG_LITE 0
#define LV_USE_DRAW_DAVE2D 0
#define LV_USE_DRAW_NEMA_GFX 0
#define LV_USE_DRAW_DMA2D 0
#define LV_USE_DRAW_PXP 0
#define LV_USE_DRAW_G2D 0
#define LV_USE_DRAW_SDL 0
#define LV_USE_DRAW_NANOVG 0
#define LV_USE_DRAW_OPENGLES 0
#define LV_USE_THORVG 0
#define LV_USE_LOG 0
#define LV_USE_ASSERT 0
#define LV_USE_SYSMON 0

/* widgets demo 需要的控件、图片、字体和布局功能。 */
#define LV_USE_IMAGE 1
#define LV_USE_IMAGEBUTTON 1
#define LV_USE_ANIMIMG 1
#define LV_USE_CANVAS 1
#define LV_USE_CHART 1
#define LV_USE_CALENDAR 1
#define LV_USE_DROPDOWN 1
#define LV_USE_KEYBOARD 1
#define LV_USE_MSGBOX 1
#define LV_USE_ROLLER 1
#define LV_USE_SCALE 1
#define LV_USE_SLIDER 1
#define LV_USE_SPAN 1
#define LV_USE_SPINBOX 1
#define LV_USE_SPINNER 1
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 1
#define LV_USE_TABLE 1
#define LV_USE_TABVIEW 1
#define LV_USE_TILEVIEW 1
#define LV_USE_ARC 1
#define LV_USE_ARCLABEL 1
#define LV_USE_BAR 1
#define LV_USE_BARCODE 1
#define LV_USE_CHECKBOX 1
#define LV_USE_LED 1
#define LV_USE_LINE 1
#define LV_USE_QRCODE 1
#define LV_USE_THEME_DEFAULT 1
#define LV_USE_THEME_SIMPLE 1
#define LV_USE_THEME_MONO 1
#define LV_USE_FLEX 1
#define LV_USE_GRID 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 1
#define LV_USE_ANIM 1

/* 当前测试官方 widgets demo；其他 demo 仍保持关闭。 */
#define LV_BUILD_EXAMPLES 0
#define LV_BUILD_DEMOS 1
#define LV_USE_DEMO_WIDGETS 1

#endif /* LV_CONF_H */
