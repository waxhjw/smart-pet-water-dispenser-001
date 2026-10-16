#include <assert.h>

#include "app/app_config.h"
#include "app/sp01_indicator.h"
#include "app/sp01_rgb_test.h"

static void assert_safe_output(sp01_outputs_t output, sp01_led_mode_t led_mode)
{
    sp01_indicator_pins_t pins;

    assert(output.pump_enable == APP_FALSE);
    assert(output.radar_power_enable == APP_FALSE);
    assert(output.led_mode == led_mode);
    assert(output.buzzer_mode == SP01_BUZZER_OFF);
    assert(output.buzzer_frequency_hz == 0U);
    assert(output.buzzer_duty_percent == 0U);

    sp01_indicator_render(output.led_mode, output.buzzer_mode, &pins);
    assert(pins.buzzer_on == APP_FALSE);
    if (led_mode != SP01_LED_OFF) {
        assert(pins.red == APP_TRUE);
        assert(pins.green == APP_TRUE);
        assert(pins.blue == APP_TRUE);
    }
}

int main(void)
{
    static const sp01_led_mode_t white_modes[] = {
        SP01_LED_TEST_WHITE_BALANCE_1,
        SP01_LED_TEST_WHITE_BALANCE_2,
        SP01_LED_TEST_WHITE_BALANCE_3,
        SP01_LED_TEST_WHITE_BALANCE_4,
        SP01_LED_TEST_WHITE_BALANCE_5,
        SP01_LED_TEST_WHITE_BALANCE_6
    };
    sp01_rgb_test_t test;
    app_u32 stage;
    app_u32 tick;

    sp01_rgb_test_init(&test);
    for (stage = 0U; stage < 6U; stage++) {
        for (tick = 0U; tick < SP01_RGB_WHITE_BALANCE_ON_MS / SP01_TICK_MS; tick++) {
            assert_safe_output(sp01_rgb_test_tick(&test), white_modes[stage]);
        }
        for (tick = 0U; tick < SP01_RGB_WHITE_BALANCE_OFF_MS / SP01_TICK_MS; tick++) {
            assert_safe_output(sp01_rgb_test_tick(&test), SP01_LED_OFF);
        }
    }

    assert_safe_output(sp01_rgb_test_tick(&test), white_modes[0]);
    return 0;
}
