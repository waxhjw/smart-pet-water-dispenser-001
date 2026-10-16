#ifndef SP01_INDICATOR_H
#define SP01_INDICATOR_H

#include "app_types.h"
#include "sp01_app.h"

/* 业务灯效到物理 RGB/蜂鸣器通道的纯软件映射。 */
typedef struct {
    app_bool_t red;
    app_bool_t green;
    app_bool_t blue;
    app_bool_t buzzer_on;
} sp01_indicator_pins_t;

void sp01_indicator_render(sp01_led_mode_t led_mode,
                           sp01_buzzer_mode_t buzzer_mode,
                           sp01_indicator_pins_t *pins);

#endif
