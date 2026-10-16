#include <assert.h>

#include "app_config.h"
#include "sp01_touch_diag_test.h"

static sp01_outputs_t tick(sp01_touch_diag_test_t *test, app_bool_t key_pressed)
{
    return sp01_touch_diag_test_update(test, key_pressed);
}

static void test_outputs_are_safe_when_idle(void)
{
    sp01_touch_diag_test_t test;
    sp01_outputs_t outputs;

    sp01_touch_diag_test_init(&test);
    outputs = tick(&test, APP_FALSE);

    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.radar_power_enable == APP_FALSE);
    assert(outputs.led_mode == SP01_LED_OFF);
    assert(outputs.buzzer_mode == SP01_BUZZER_OFF);
}

static void test_pb0_high_is_green_and_safe(void)
{
    sp01_touch_diag_test_t test;
    sp01_outputs_t outputs;

    sp01_touch_diag_test_init(&test);
    outputs = tick(&test, APP_TRUE);
    assert(outputs.led_mode == SP01_LED_TEST_PA5);
    assert(outputs.buzzer_mode == SP01_BUZZER_OFF);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.radar_power_enable == APP_FALSE);

    outputs = tick(&test, APP_TRUE);
    assert(outputs.led_mode == SP01_LED_TEST_PA5);
    assert(outputs.buzzer_mode == SP01_BUZZER_OFF);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.radar_power_enable == APP_FALSE);
}

static void test_pb0_edges_beep_and_low_level_is_off(void)
{
    sp01_touch_diag_test_t test;
    sp01_outputs_t outputs;

    sp01_touch_diag_test_init(&test);
    outputs = tick(&test, APP_FALSE);
    assert(outputs.led_mode == SP01_LED_OFF);
    assert(outputs.buzzer_mode == SP01_BUZZER_OFF);

    outputs = tick(&test, APP_TRUE);
    assert(outputs.led_mode == SP01_LED_TEST_PA5);
    assert(outputs.buzzer_mode == SP01_BUZZER_KEY_CLICK);

    outputs = tick(&test, APP_FALSE);
    assert(outputs.led_mode == SP01_LED_OFF);
    assert(outputs.buzzer_mode == SP01_BUZZER_KEY_CLICK);

    outputs = tick(&test, APP_FALSE);
    assert(outputs.led_mode == SP01_LED_OFF);
    assert(outputs.buzzer_mode == SP01_BUZZER_OFF);
}

int main(void)
{
    test_outputs_are_safe_when_idle();
    test_pb0_high_is_green_and_safe();
    test_pb0_edges_beep_and_low_level_is_off();
    return 0;
}
