#include <assert.h>

#include "app/sp01_pump_adc_hold_test.h"

int main(void)
{
    sp01_pump_adc_hold_test_t test;
    sp01_outputs_t outputs;

    sp01_pump_adc_hold_test_init(&test);

    /* 上电时即使按键仍被按住，也不允许自动启动水泵。 */
    outputs = sp01_pump_adc_hold_test_update(&test, APP_FALSE, APP_TRUE);
    assert(outputs.pump_enable == APP_FALSE);

    /* 去抖确认按住期间仍不能启动；实际松开并稳定后才进入待机。 */
    outputs = sp01_pump_adc_hold_test_update(&test, APP_TRUE, APP_TRUE);
    assert(outputs.pump_enable == APP_FALSE);
    outputs = sp01_pump_adc_hold_test_update(&test, APP_FALSE, APP_FALSE);
    assert(outputs.pump_enable == APP_FALSE);
    outputs = sp01_pump_adc_hold_test_update(&test, APP_TRUE, APP_TRUE);
    assert(outputs.pump_enable == APP_TRUE);
    assert(outputs.radar_power_enable == APP_FALSE);
    assert(outputs.buzzer_mode == SP01_BUZZER_OFF);

    /* 按住不会反复切换；松开后再按一次才停止。 */
    outputs = sp01_pump_adc_hold_test_update(&test, APP_TRUE, APP_TRUE);
    assert(outputs.pump_enable == APP_TRUE);
    outputs = sp01_pump_adc_hold_test_update(&test, APP_FALSE, APP_FALSE);
    assert(outputs.pump_enable == APP_TRUE);
    outputs = sp01_pump_adc_hold_test_update(&test, APP_TRUE, APP_TRUE);
    assert(outputs.pump_enable == APP_FALSE);

    return 0;
}
