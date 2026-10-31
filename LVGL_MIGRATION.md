# LVGL 接入说明

当前工程使用 `lvgl` 目录中的 LVGL 10.0.0-dev 完整源码。该目录包含完整的
`src`、`examples` 和 `demos`，因此只复制 `Screen_test_8080` 目录即可独立构建。

外部 `STM32Projects/lvgl-master` 中先前修改的 4 处 `max_buf_size` 调用已恢复。
项目内部的 `lvgl` 与该原包保持一致，也保留了 10.0.0-dev 原有的变量名错误。
`cmake/lvgl.cmake` 在你配置工程时，将出错的 `lv_draw_sw_img.c` 复制到 build 目录，
只在该临时文件中将 `max_buf_size(t->target_layer)` 修正为 `max_buf_size(layer)`，
并让 LVGL 编译这个临时文件。这样修复随工程一起保存，两个源码包均保持原样。

## 当前运行界面

- `Core/Src/lvgl_port.c` 创建 480×320 的 RGB565 显示器，使用 20 行局部绘图缓冲。
- 当前启动官方 widgets demo；原来的纯色背景和 `touch` 按钮代码仍保留在 `#if 0` 中。
- FT6336 坐标继续使用 `touch_ft6336.h` 中的轴交换、X/Y 反转和 LCD 镜像参数。
- LVGL 只由 `lvglTask` 调用，`LV_USE_OS` 设置为 `LV_OS_NONE`，避免多个任务同时访问 LVGL。

## 内存和配置

`lv_conf.h` 将 LVGL 内存池设为 64 KB，显示缓冲约 19.2 KB；FreeRTOS 堆调整为 24 KB，
widgets 任务栈调整为 12 KB。F407 的 RAM 较紧，启用官方 demo、较大字体或图片前应先查看链接器的
RAM 使用量，并相应减小 `LV_MEM_SIZE`、绘图缓冲或任务栈。

当前已经启用并启动 widgets demo。后续切换其他官方 demo 时，需要在
`lv_conf.h` 打开对应的 `LV_USE_DEMO_*` 以及它需要的控件、字体等配置，
再在 `lvgl_port.c` 中调用相应 demo 的创建函数。

工程按用户要求未执行编译；请在本机编译后观察屏幕和触摸坐标。若四角坐标方向不一致，
可在 `touch_ft6336.h` 检查安装方向补偿（`FT6336_INVERT_X/Y`）；
`FT6336_SWAP_XY` 需与 `LCD8080_LANDSCAPE` 一致。LCD 镜像参数会自动叠加到触摸映射。
