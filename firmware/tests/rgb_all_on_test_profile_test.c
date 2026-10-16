#include <assert.h>

#include "app/sp01_indicator.h"
#include "app/sp01_rgb_test.h"

int main(void)
{
    sp01_rgb_test_t test;
    sp01_indicator_pins_t pins;
    app_u32 index;

    sp01_rgb_test_init(&test);
    for (index = 0UL; index < 1000UL; index++) {
        sp01_outputs_t output = sp01_rgb_test_tick(&test);

        assert(output.pump_enable == APP_FALSE);
        assert(output.radar_power_enable == APP_FALSE);
        assert(output.led_mode == SP01_LED_TEST_COMBO_ALL);
        assert(output.buzzer_mode == SP01_BUZZER_OFF);
        assert(output.buzzer_frequency_hz == 0U);

        sp01_indicator_render(output.led_mode, output.buzzer_mode, &pins);
        assert(pins.red == APP_TRUE);
        assert(pins.green == APP_TRUE);
        assert(pins.blue == APP_TRUE);
        assert(pins.buzzer_on == APP_FALSE);
    }

    return 0;
}
