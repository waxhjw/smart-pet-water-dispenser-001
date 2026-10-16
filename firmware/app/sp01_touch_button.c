#include "sp01_touch_button.h"

void sp01_touch_button_init(sp01_touch_button_t *button)
{
    if (button == 0) {
        return;
    }

    button->was_pressed = APP_FALSE;
    button->was_long_signal = APP_FALSE;
    button->pressed_elapsed_ms = 0UL;
    button->long_reported = APP_FALSE;
}

sp01_touch_button_event_t sp01_touch_button_update(sp01_touch_button_t *button,
                                                   app_bool_t pressed,
                                                   app_bool_t external_long_signal,
                                                   app_u32 tick_ms,
                                                   app_u32 long_press_ms)
{
    sp01_touch_button_event_t event;
    app_bool_t was_pressed;

    event.pressed_edge = APP_FALSE;
    event.short_release = APP_FALSE;
    event.long_edge = APP_FALSE;
    event.released_duration_ms = 0UL;

    if (button == 0) {
        return event;
    }

    was_pressed = button->was_pressed;

    if (pressed == APP_TRUE && was_pressed == APP_FALSE) {
        event.pressed_edge = APP_TRUE;
    }

    /* 如果触摸芯片未来提供独立长按信号，这里直接兼容。 */
    if (external_long_signal == APP_TRUE &&
        button->was_long_signal == APP_FALSE) {
        event.long_edge = APP_TRUE;
        button->long_reported = APP_TRUE;
    }

    if (pressed == APP_TRUE) {
        if (button->pressed_elapsed_ms < long_press_ms) {
            button->pressed_elapsed_ms += tick_ms;
        }

        /* 当前硬件只有 KEY 电平时，由 MCU 自己累计长按时间。 */
        if (button->pressed_elapsed_ms >= long_press_ms &&
            button->long_reported == APP_FALSE) {
            event.long_edge = APP_TRUE;
            button->long_reported = APP_TRUE;
        }
    } else {
        event.released_duration_ms = button->pressed_elapsed_ms;
        /*
         * 短按在松手时确认，避免长按先触发一次手动出水。
         * 例如：按下 -> 松手 = short_release；按下并保持 2 秒 = long_edge。
         */
        if (was_pressed == APP_TRUE &&
            button->long_reported == APP_FALSE &&
            button->pressed_elapsed_ms > 0UL) {
            event.short_release = APP_TRUE;
        }

        button->pressed_elapsed_ms = 0UL;
        button->long_reported = APP_FALSE;
    }

    button->was_pressed = pressed;
    button->was_long_signal = external_long_signal;

    return event;
}
