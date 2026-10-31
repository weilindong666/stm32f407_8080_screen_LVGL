# LVGL is kept as an unmodified, self-contained source tree in ../lvgl.
# The upstream 10.0.0-dev tree has a typo in lv_draw_sw_img.c.  Generate a
# corrected build-only copy instead of changing the source package.

# 保留官方 CMake 的源码与依赖设置，仅替换有错误的一个编译单元。
get_target_property(LVGL_SOURCES lvgl SOURCES)
set(LVGL_SW_IMG_SOURCE "${LVGL_ROOT_DIR}/src/draw/sw/lv_draw_sw_img.c")
set(LVGL_SW_IMG_BUILD_COPY "${CMAKE_CURRENT_BINARY_DIR}/lvgl_sw_img_fixed.c")
list(REMOVE_ITEM LVGL_SOURCES "${LVGL_SW_IMG_SOURCE}")
configure_file("${LVGL_SW_IMG_SOURCE}" "${LVGL_SW_IMG_BUILD_COPY}" COPYONLY)
file(READ "${LVGL_SW_IMG_BUILD_COPY}" LVGL_SW_IMG_TEXT)
string(REPLACE "max_buf_size(t->target_layer)"
               "max_buf_size(layer)"
               LVGL_SW_IMG_TEXT "${LVGL_SW_IMG_TEXT}")
file(WRITE "${LVGL_SW_IMG_BUILD_COPY}" "${LVGL_SW_IMG_TEXT}")
list(APPEND LVGL_SOURCES "${LVGL_SW_IMG_BUILD_COPY}")
set_property(TARGET lvgl PROPERTY SOURCES "${LVGL_SOURCES}")
# 临时文件移到 build 后，其相对头文件仍从原 sw 目录解析。
set_source_files_properties("${LVGL_SW_IMG_BUILD_COPY}" TARGET_DIRECTORY lvgl PROPERTIES
    INCLUDE_DIRECTORIES "${LVGL_ROOT_DIR}/src/draw/sw")
