#include "sp01_pump_adc_hold_test.h"

static sp01_outputs_t sp01_pump_adc_hold_test_outputs(app_bool_t pump_enabled)
{
    sp01_outputs_t outputs;

    outputs.pump_enable = pump_enabled;
    outputs.radar_power_enable = APP_FALSE;
    outputs.led_mode = SP01_LED_OFF;
    outputs.buzzer_mode = SP01_BUZZER_OFF;
    outputs.buzzer_frequency_hz = 0U;
    outputs.buzzer_duty_percent = 0U;
    return outputs;
}

void sp01_pump_adc_hold_test_init(sp01_pump_adc_hold_test_t *test)
{
    if (test == 0) {
        return;
    }

    test->pump_enabled = APP_FALSE;
    test->key_was_pressed = APP_FALSE;
    test->armed = APP_FALSE;
}

sp01_outputs_t sp01_pump_adc_hold_test_update(sp01_pump_adc_hold_test_t *test,
                                              app_bool_t key_pressed,
                                              app_bool_t raw_key_pressed)
{
    if (test == 0) {
        return sp01_pump_adc_hold_test_outputs(APP_FALSE);
    }

    if (raw_key_pressed == APP_FALSE && key_pressed == APP_FALSE) {
        test->armed = APP_TRUE;
    } else if (test->armed == APP_TRUE &&
               test->key_was_pressed == APP_FALSE) {
        test->pump_enabled = test->pump_enabled == APP_TRUE ?
                             APP_FALSE : APP_TRUE;
    }

    test->key_was_pressed = key_pressed;
    return sp01_pump_adc_hold_test_outputs(test->pump_enabled);
}
