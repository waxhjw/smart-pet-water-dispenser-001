#include "sp01_touch_diag_test.h"
#include "app_config.h"

void sp01_touch_diag_test_init(sp01_touch_diag_test_t *test)
{
    if (test == 0) {
        return;
    }

    test->initialized = APP_FALSE;
    test->last_key_pressed = APP_FALSE;
}

sp01_outputs_t sp01_touch_diag_test_update(sp01_touch_diag_test_t *test,
                                           app_bool_t key_pressed)
{
    sp01_outputs_t outputs;

    outputs.pump_enable = APP_FALSE;
    outputs.radar_power_enable = APP_FALSE;
    outputs.led_mode = SP01_LED_OFF;
    outputs.buzzer_mode = SP01_BUZZER_OFF;
    outputs.buzzer_frequency_hz = 0U;
    outputs.buzzer_duty_percent = 0U;

    if (test == 0) {
        return outputs;
    }

    if (test->initialized == APP_FALSE) {
        test->initialized = APP_TRUE;
        test->last_key_pressed = key_pressed;
    } else if (key_pressed != test->last_key_pressed) {
        outputs.buzzer_mode = SP01_BUZZER_KEY_CLICK;
        test->last_key_pressed = key_pressed;
    }

    if (key_pressed == APP_TRUE) {
        outputs.led_mode = SP01_LED_TEST_PA5;
    }

    /* PB0 变化时短响标记边沿；泵和雷达输出始终关闭。 */
    if (key_pressed == APP_FALSE) {
        outputs.led_mode = SP01_LED_OFF;
    }

    return outputs;
}
