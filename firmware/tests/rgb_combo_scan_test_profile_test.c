#include <assert.h>

#include "app/app_config.h"
#include "app/sp01_rgb_test.h"

int main(void)
{
    sp01_rgb_test_t test;
    sp01_outputs_t output;
    app_u32 index;

    sp01_rgb_test_init(&test);
    for (index = 0UL; index < 8UL; index++) {
        output = sp01_rgb_test_tick(&test);
        assert(output.pump_enable == APP_FALSE);
        assert(output.radar_power_enable == APP_FALSE);
        assert(output.buzzer_mode == SP01_BUZZER_OFF);
        for (app_u32 tick = 1UL;
             tick < (SP01_RGB_COMBO_SCAN_STEP_MS / SP01_TICK_MS);
             tick++) {
            output = sp01_rgb_test_tick(&test);
        }
    }
    return 0;
}
