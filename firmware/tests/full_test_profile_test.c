#include <assert.h>

#include "app/app_config.h"
#include "app/sp01_full_test.h"

static sp01_full_test_inputs_t inputs(void)
{
    sp01_full_test_inputs_t value;

    value.key_pressed = APP_FALSE;
    value.radar_active = APP_FALSE;
    value.radar_rising_edge = APP_FALSE;
    value.pump_diag = SP01_PUMP_DIAG_UNKNOWN;
    value.injected_fault = 0U;
    return value;
}

static sp01_outputs_t tick(sp01_full_test_t *test,
                           sp01_full_test_inputs_t *value)
{
    return sp01_full_test_tick(test, value);
}

static void short_click(sp01_full_test_t *test)
{
    sp01_full_test_inputs_t value = inputs();

    value.key_pressed = APP_TRUE;
    (void)tick(test, &value);
    value.key_pressed = APP_FALSE;
    (void)tick(test, &value);
}

static void wait_ticks(sp01_full_test_t *test, app_u32 count)
{
    sp01_full_test_inputs_t value = inputs();
    app_u32 index;

    for (index = 0UL; index < count; index++) {
        (void)tick(test, &value);
    }
}

static void hold_key(sp01_full_test_t *test, app_u32 count)
{
    sp01_full_test_inputs_t value = inputs();
    app_u32 index;

    value.key_pressed = APP_TRUE;
    for (index = 0UL; index < count; index++) {
        (void)tick(test, &value);
    }
}

int main(void)
{
    sp01_full_test_t test;
    sp01_full_test_t startup_radar_test;
    sp01_full_test_inputs_t value;
    sp01_outputs_t output;
    app_u32 index;

    sp01_full_test_init(&test);
    sp01_full_test_init(&startup_radar_test);
    value = inputs();

    /* 上电时雷达若已经为高，不应把稳定高电平当成新触发。 */
    value.radar_active = APP_TRUE;
    for (index = 0UL; index < (SP01_FULL_TEST_BOOT_SETTLE_MS / SP01_TICK_MS); index++) {
        (void)tick(&startup_radar_test, &value);
    }
    value.radar_rising_edge = APP_TRUE;
    output = tick(&startup_radar_test, &value);
    assert(output.pump_enable == APP_FALSE);
    value.radar_active = APP_FALSE;
    value.radar_rising_edge = APP_FALSE;
    (void)tick(&startup_radar_test, &value);
    value.radar_active = APP_TRUE;
    value.radar_rising_edge = APP_TRUE;
    output = tick(&startup_radar_test, &value);
    assert(output.pump_enable == APP_TRUE);

    value = inputs();
    wait_ticks(&test, SP01_FULL_TEST_BOOT_SETTLE_MS / SP01_TICK_MS);
    assert(test.power_state == SP01_FULL_POWER_ON_SENSOR);

    short_click(&test);
    wait_ticks(&test, (SP01_FULL_TEST_DOUBLE_CLICK_MS / SP01_TICK_MS) + 1UL);
    output = tick(&test, &value);
    assert(output.pump_enable == APP_TRUE);
    assert(output.led_mode == SP01_LED_SENSOR_PUMPING);
    wait_ticks(&test, SP01_FULL_TEST_PUMP_RUN_MS / SP01_TICK_MS);
    output = tick(&test, &value);
    assert(output.pump_enable == APP_FALSE);

    value.radar_rising_edge = APP_TRUE;
    output = tick(&test, &value);
    assert(output.pump_enable == APP_TRUE);
    value.radar_rising_edge = APP_FALSE;
    wait_ticks(&test, SP01_FULL_TEST_PUMP_RUN_MS / SP01_TICK_MS);
    output = tick(&test, &value);
    assert(output.pump_enable == APP_FALSE);
    assert(test.radar_cooldown_ms > 0UL);

    value.radar_rising_edge = APP_TRUE;
    output = tick(&test, &value);
    assert(output.pump_enable == APP_FALSE);
    value.radar_rising_edge = APP_FALSE;
    wait_ticks(&test, SP01_FULL_TEST_RADAR_COOLDOWN_MS / SP01_TICK_MS);

    short_click(&test);
    short_click(&test);
    output = tick(&test, &value);
    assert(test.power_state == SP01_FULL_POWER_ON_CONTINUOUS);
    assert(output.pump_enable == APP_TRUE);
    assert(output.led_mode == SP01_LED_CONTINUOUS);
    short_click(&test);
    short_click(&test);
    output = tick(&test, &value);
    assert(test.power_state == SP01_FULL_POWER_ON_SENSOR);
    assert(output.pump_enable == APP_FALSE);

    hold_key(&test, SP01_FULL_TEST_NIGHT_PRESS_MS / SP01_TICK_MS);
    value.key_pressed = APP_FALSE;
    output = tick(&test, &value);
    assert(test.night_mode_enabled == APP_TRUE);
    assert(output.led_mode == SP01_LED_OFF);

    hold_key(&test, SP01_FULL_TEST_POWER_PRESS_MS / SP01_TICK_MS);
    (void)tick(&test, &value);
    assert(test.power_state == SP01_FULL_POWER_OFF);

    /* 低水位诊断必须立即停泵，并在 ADC 恢复未知后保持锁定。 */
    {
        sp01_full_test_t low_water_test;

        sp01_full_test_init(&low_water_test);
        value = inputs();
        wait_ticks(&low_water_test, SP01_FULL_TEST_BOOT_SETTLE_MS / SP01_TICK_MS);
        short_click(&low_water_test);
        wait_ticks(&low_water_test,
                   (SP01_FULL_TEST_DOUBLE_CLICK_MS / SP01_TICK_MS) + 1UL);
        output = tick(&low_water_test, &value);
        assert(output.pump_enable == APP_TRUE);

        value.pump_diag = SP01_PUMP_DIAG_LOW_WATER_SUSPECT;
        output = tick(&low_water_test, &value);
        assert(output.pump_enable == APP_FALSE);
        assert(low_water_test.low_water_active == APP_TRUE);
        assert(output.buzzer_mode == SP01_BUZZER_OFF);

        value = inputs();
        output = tick(&low_water_test, &value);
        assert(output.pump_enable == APP_FALSE);
        assert(low_water_test.low_water_active == APP_TRUE);
        assert(output.buzzer_mode == SP01_BUZZER_LOW_WATER);

        /* 锁定期间短按不得重新启动水泵。 */
        short_click(&low_water_test);
        wait_ticks(&low_water_test,
                   (SP01_FULL_TEST_DOUBLE_CLICK_MS / SP01_TICK_MS) + 1UL);
        output = tick(&low_water_test, &value);
        assert(output.pump_enable == APP_FALSE);
        assert(low_water_test.low_water_active == APP_TRUE);
    }

    return 0;
}
