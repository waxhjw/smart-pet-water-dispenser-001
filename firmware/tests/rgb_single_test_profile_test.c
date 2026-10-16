#include <assert.h>

#include "app/sp01_rgb_test.h"

int main(void)
{
    sp01_rgb_test_t test;
    sp01_outputs_t output;
    app_u32 index;

    sp01_rgb_test_init(&test);
    for (index = 0UL; index < 20UL; index++) {
        output = sp01_rgb_test_tick(&test);
        assert(output.pump_enable == APP_FALSE);
        assert(output.radar_power_enable == APP_FALSE);
        assert(output.led_mode == SP01_LED_TEST_PA5);
        assert(output.buzzer_mode == SP01_BUZZER_OFF);
        assert(output.buzzer_frequency_hz == 0U);
    }

    return 0;
}
