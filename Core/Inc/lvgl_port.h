#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

void LVGL_Port_Init(void);
void LVGL_Port_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* LVGL_PORT_H */
