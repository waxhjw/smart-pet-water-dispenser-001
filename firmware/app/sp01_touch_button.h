#ifndef SP01_TOUCH_BUTTON_H
#define SP01_TOUCH_BUTTON_H

#include "app_types.h"

/*
 * 触摸按键输入状态。
 * 业务层不直接判断电平持续时间，而是读取下面的按下/短按/长按事件。
 */
typedef struct {
    app_bool_t was_pressed;
    app_bool_t was_long_signal;
    app_u32 pressed_elapsed_ms;
    app_bool_t long_reported;
} sp01_touch_button_t;

typedef struct {
    app_bool_t pressed_edge;
    app_bool_t short_release;
    app_bool_t long_edge;
    /* 松手时返回本次按下持续时间；长按也会返回饱和值。 */
    app_u32 released_duration_ms;
} sp01_touch_button_event_t;

void sp01_touch_button_init(sp01_touch_button_t *button);
sp01_touch_button_event_t sp01_touch_button_update(sp01_touch_button_t *button,
                                                   app_bool_t pressed,
                                                   app_bool_t external_long_signal,
                                                   app_u32 tick_ms,
                                                   app_u32 long_press_ms);

#endif
