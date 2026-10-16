#ifndef SP01_TOUCH_DIAG_TEST_H
#define SP01_TOUCH_DIAG_TEST_H

#include "sp01_app.h"

typedef struct {
    app_bool_t initialized;
    app_bool_t last_key_pressed;
} sp01_touch_diag_test_t;

void sp01_touch_diag_test_init(sp01_touch_diag_test_t *test);

/*
 * 触摸信号诊断：水泵和雷达供电始终关闭。
 * 按键有效时绿灯常亮，按键状态变化时短响一次。
 * 板级配置会把实测 PB0 低电平转换为“按键有效”。
 */
sp01_outputs_t sp01_touch_diag_test_update(sp01_touch_diag_test_t *test,
                                           app_bool_t key_pressed);

#endif
