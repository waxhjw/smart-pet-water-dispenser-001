#include <assert.h>

#include "app/app_config.h"
#include "app/sp01_rgb_test.h"

int main(void)
{
    sp01_rgb_test_t test;
    sp01_outputs_t output;
    app_u32 index;

    sp01_rgb_test_init(&test);
    output = sp01_rgb_test_tick(&test);
    assert(output.led_mode == SP01_LED_TEST_COMBO_PA5_PA1);
    assert(output.pump_enable == APP_FALSE);
    assert(output.radar_power_enable == APP_FALSE);
    assert(output.buzzer_mode == SP01_BUZZER_OFF);

    for (index = 1UL; index < (SP01_RGB_TEST_CHANNEL_ON_MS / SP01_TICK_MS); index++) {
        output = sp01_rgb_test_tick(&test);
        assert(output.led_mode == SP01_LED_TEST_COMBO_PA5_PA1);
    }

    output = sp01_rgb_test_tick(&test);
    assert(output.led_mode == SP01_LED_OFF);
    return 0;
}
