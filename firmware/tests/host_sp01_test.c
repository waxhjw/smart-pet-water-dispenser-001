#include <assert.h>
#include <stdio.h>

#include "app/app_config.h"
#include "app/sp01_app.h"
#include "app/sp01_buzzer_pattern.h"
#include "app/sp01_indicator.h"
#include "app/sp01_power_monitor.h"
#include "app/sp01_pump_monitor.h"
#include "app/sp01_radar_input.h"
#include "app/sp01_charge_monitor.h"
#include "app/sp01_signal_filter.h"
#include "app/sp01_touch_button.h"
#include "platform/py32f002b/board_port.h"

static sp01_inputs_t make_inputs(void)
{
    sp01_inputs_t inputs;

    inputs.radar_active = APP_FALSE;
    inputs.key_pressed = APP_FALSE;
    inputs.key_long_pressed = APP_FALSE;
    inputs.charging_active = APP_FALSE;
    inputs.charge_done = APP_FALSE;
    inputs.battery_low = APP_FALSE;
    inputs.pump_diag = SP01_PUMP_DIAG_UNKNOWN;

    return inputs;
}

typedef struct {
    sp01_app_t app;
    sp01_pump_monitor_t pump_monitor;
    sp01_bool_filter_t radar_filter;
    sp01_outputs_t last_outputs;
} sim_runtime_t;

static void sim_runtime_init(sim_runtime_t *runtime)
{
    sp01_app_init(&runtime->app);
    sp01_pump_monitor_init(&runtime->pump_monitor);
    sp01_bool_filter_init(&runtime->radar_filter, APP_FALSE);
    runtime->last_outputs.pump_enable = APP_FALSE;
    runtime->last_outputs.radar_power_enable = APP_FALSE;
    runtime->last_outputs.led_mode = SP01_LED_OFF;
    runtime->last_outputs.buzzer_mode = SP01_BUZZER_OFF;
}

static sp01_outputs_t sim_runtime_tick(sim_runtime_t *runtime,
                                       app_bool_t raw_radar,
                                       app_u16 pump_adc_mv)
{
    sp01_inputs_t inputs;
    sp01_outputs_t outputs;

    inputs = make_inputs();
    inputs.radar_active = sp01_bool_filter_update(&runtime->radar_filter,
                                                  raw_radar,
                                                  SP01_RADAR_DEBOUNCE_TICKS);
    inputs.pump_diag = sp01_pump_monitor_update(&runtime->pump_monitor,
                                                runtime->last_outputs.pump_enable,
                                                pump_adc_mv);
    outputs = sp01_app_tick(&runtime->app, &inputs);
    runtime->last_outputs = outputs;
    return outputs;
}

static sp01_outputs_t tick_app(sp01_app_t *app, sp01_inputs_t *inputs, app_u32 ticks)
{
    sp01_outputs_t outputs;
    app_u32 i;

    outputs.pump_enable = APP_FALSE;
    outputs.radar_power_enable = APP_FALSE;
    outputs.led_mode = SP01_LED_OFF;
    outputs.buzzer_mode = SP01_BUZZER_OFF;

    for (i = 0; i < ticks; i++) {
        outputs = sp01_app_tick(app, inputs);
    }

    return outputs;
}

static void test_boot_defaults_safe(void)
{
    sp01_app_t app;
    sp01_inputs_t inputs;
    sp01_outputs_t outputs;

    sp01_app_init(&app);
    inputs = make_inputs();
    outputs = sp01_app_tick(&app, &inputs);

    assert(app.state == SP01_STATE_BOOT);
    assert(app.fault_code == SP01_FAULT_NONE);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.radar_power_enable == APP_FALSE);
    assert(outputs.buzzer_mode == SP01_BUZZER_OFF);

    tick_app(&app, &inputs, SP01_BOOT_SETTLE_MS / SP01_TICK_MS);
    assert(app.state == SP01_STATE_STANDBY);
    outputs = sp01_app_tick(&app, &inputs);
    assert(outputs.radar_power_enable == APP_TRUE);
}

static void test_radar_run_then_lockout(void)
{
    sp01_app_t app;
    sp01_inputs_t inputs;
    sp01_outputs_t outputs;

    sp01_app_init(&app);
    inputs = make_inputs();
    tick_app(&app, &inputs, (SP01_BOOT_SETTLE_MS / SP01_TICK_MS) + 1U);

    inputs.radar_active = APP_TRUE;
    outputs = sp01_app_tick(&app, &inputs);
    assert(app.state == SP01_STATE_SENSOR_RUN);
    assert(outputs.pump_enable == APP_FALSE);

    inputs.radar_active = APP_FALSE;
    outputs = sp01_app_tick(&app, &inputs);
    assert(outputs.pump_enable == APP_TRUE);

    outputs = tick_app(&app, &inputs, SP01_SENSOR_RUN_MS / SP01_TICK_MS);
    assert(app.state == SP01_STATE_LOCKOUT);
    assert(outputs.pump_enable == APP_FALSE);

    outputs = sp01_app_tick(&app, &inputs);
    assert(outputs.pump_enable == APP_FALSE);
}

static void test_radar_must_return_idle_after_boot_before_starting_pump(void)
{
    sp01_app_t app;
    sp01_inputs_t inputs;
    sp01_outputs_t outputs;

    sp01_app_init(&app);
    inputs = make_inputs();
    inputs.radar_active = APP_TRUE;
    tick_app(&app, &inputs, (SP01_BOOT_SETTLE_MS / SP01_TICK_MS) + 2U);

    assert(app.state == SP01_STATE_STANDBY);
    outputs = sp01_app_tick(&app, &inputs);
    assert(app.state == SP01_STATE_STANDBY);
    assert(outputs.pump_enable == APP_FALSE);

    inputs.radar_active = APP_FALSE;
    outputs = sp01_app_tick(&app, &inputs);
    assert(outputs.pump_enable == APP_FALSE);

    inputs.radar_active = APP_TRUE;
    outputs = sp01_app_tick(&app, &inputs);
    assert(app.state == SP01_STATE_SENSOR_RUN);
    assert(outputs.pump_enable == APP_FALSE);
}

static void test_low_water_requires_two_failed_checks(void)
{
    sp01_app_t app;
    sp01_inputs_t inputs;
    sp01_outputs_t outputs;

    sp01_app_init(&app);
    inputs = make_inputs();
    tick_app(&app, &inputs, (SP01_BOOT_SETTLE_MS / SP01_TICK_MS) + 1U);

    inputs.radar_active = APP_TRUE;
    sp01_app_tick(&app, &inputs);
    inputs.radar_active = APP_FALSE;

    inputs.pump_diag = SP01_PUMP_DIAG_LOW_WATER_SUSPECT;
    outputs = sp01_app_tick(&app, &inputs);
    assert(app.state == SP01_STATE_LOW_WATER_SUSPECT);
    assert(app.fault_code == SP01_FAULT_LOW_WATER);
    assert(outputs.pump_enable == APP_FALSE);

    inputs.pump_diag = SP01_PUMP_DIAG_UNKNOWN;
    tick_app(&app, &inputs, (SP01_LOW_WATER_RETRY_OFF_MS / SP01_TICK_MS) + 1U);
    assert(app.state == SP01_STATE_SENSOR_RUN);

    inputs.pump_diag = SP01_PUMP_DIAG_LOW_WATER_SUSPECT;
    sp01_app_tick(&app, &inputs);
    assert(app.state == SP01_STATE_LOW_WATER_ALARM);
    assert(app.fault_code == SP01_FAULT_LOW_WATER);
}

static void test_pump_monitor_classification(void)
{
    sp01_pump_monitor_t monitor;
    sp01_pump_diag_t diag;
    app_u32 i;

    sp01_pump_monitor_init(&monitor);

    for (i = 0; i < (SP01_PUMP_START_IGNORE_MS + SP01_PUMP_DIAG_WINDOW_MS) / SP01_TICK_MS; i++) {
        diag = sp01_pump_monitor_update(&monitor, APP_TRUE, 100U);
    }
    assert(diag == SP01_PUMP_DIAG_LOW_WATER_SUSPECT);

    sp01_pump_monitor_update(&monitor, APP_FALSE, 0U);
    for (i = 0; i < (SP01_PUMP_START_IGNORE_MS + SP01_PUMP_DIAG_WINDOW_MS) / SP01_TICK_MS; i++) {
        diag = sp01_pump_monitor_update(&monitor, APP_TRUE, 180U);
    }
    assert(diag == SP01_PUMP_DIAG_NORMAL);

    sp01_pump_monitor_update(&monitor, APP_FALSE, 0U);
    for (i = 0; i < (SP01_PUMP_START_IGNORE_MS + SP01_PUMP_DIAG_WINDOW_MS) / SP01_TICK_MS; i++) {
        diag = sp01_pump_monitor_update(&monitor, APP_TRUE, 20U);
    }
    assert(diag == SP01_PUMP_DIAG_NO_LOAD);
}

static void test_power_monitor_classification_and_debounce(void)
{
    sp01_power_monitor_t monitor;
    sp01_power_snapshot_t snapshot;
    app_u32 i;

    sp01_power_monitor_init(&monitor);
    assert(sp01_power_monitor_update(&monitor, 3800U) == SP01_POWER_DIAG_UNKNOWN);

    for (i = 0U; i < SP01_BATTERY_FAULT_CONFIRM_TICKS; i++) {
        assert(sp01_power_monitor_update(&monitor, 3350U) ==
               (i + 1U < SP01_BATTERY_FAULT_CONFIRM_TICKS ?
                SP01_POWER_DIAG_UNKNOWN : SP01_POWER_DIAG_LOW));
    }
    snapshot = sp01_power_monitor_snapshot(&monitor);
    assert(snapshot.battery_mv == 3350U);
    assert(snapshot.raw_diag == SP01_POWER_DIAG_LOW);
    assert(snapshot.diag == SP01_POWER_DIAG_LOW);

    for (i = 0U; i < SP01_BATTERY_RECOVERY_CONFIRM_TICKS; i++) {
        (void)sp01_power_monitor_update(&monitor, 3800U);
    }
    assert(monitor.diag == SP01_POWER_DIAG_NORMAL);

    for (i = 0U; i < SP01_BATTERY_FAULT_CONFIRM_TICKS; i++) {
        (void)sp01_power_monitor_update(&monitor, 0U);
    }
    assert(monitor.diag == SP01_POWER_DIAG_INVALID);
    assert(sp01_power_diag_name(SP01_POWER_DIAG_CRITICAL)[0] == 'C');
}

static void test_buzzer_pattern_is_non_blocking(void)
{
    sp01_buzzer_pattern_t pattern;
    sp01_buzzer_mode_t mode;
    app_u32 i;

    sp01_buzzer_pattern_init(&pattern);
    assert(sp01_buzzer_pattern_update(&pattern, SP01_BUZZER_OFF) == SP01_BUZZER_OFF);

    mode = sp01_buzzer_pattern_update(&pattern, SP01_BUZZER_KEY_CLICK);
    assert(mode == SP01_BUZZER_KEY_CLICK);
    for (i = 1U; i < (SP01_BUZZER_KEY_CLICK_MS / SP01_TICK_MS) + 1U; i++) {
        mode = sp01_buzzer_pattern_update(&pattern, SP01_BUZZER_OFF);
    }
    assert(mode == SP01_BUZZER_OFF);

    mode = sp01_buzzer_pattern_update(&pattern, SP01_BUZZER_RADAR_TEST);
    assert(mode == SP01_BUZZER_RADAR_TEST);
    for (i = 1U; i < (SP01_BUZZER_RADAR_TEST_MS / SP01_TICK_MS) + 1U; i++) {
        mode = sp01_buzzer_pattern_update(&pattern, SP01_BUZZER_OFF);
    }
    assert(mode == SP01_BUZZER_OFF);

    mode = sp01_buzzer_pattern_update(&pattern, SP01_BUZZER_FAULT);
    assert(mode == SP01_BUZZER_FAULT);
    for (i = 0U; i < (SP01_BUZZER_FAULT_ON_MS / SP01_TICK_MS); i++) {
        mode = sp01_buzzer_pattern_update(&pattern, SP01_BUZZER_FAULT);
    }
    assert(mode == SP01_BUZZER_OFF);
    sp01_buzzer_pattern_update(&pattern, SP01_BUZZER_OFF);
    assert(pattern.active_mode == SP01_BUZZER_OFF);
}

static void test_power_buzzer_duty_matches_spec(void)
{
    sp01_buzzer_pattern_t pattern;
    app_u16 frequency_hz[3];
    app_u8 duty_percent[3];
    app_u8 tone_ticks[3];
    app_u8 count;
    app_u32 i;
    app_u16 current_frequency_hz;
    app_u16 previous_frequency_hz;

    sp01_buzzer_pattern_init(&pattern);
    count = 0U;
    previous_frequency_hz = 0U;
    tone_ticks[0] = 0U;
    tone_ticks[1] = 0U;
    tone_ticks[2] = 0U;
    for (i = 0U; i < 40U; i++) {
        (void)sp01_buzzer_pattern_update(&pattern, SP01_BUZZER_POWER_ON);
        current_frequency_hz = sp01_buzzer_pattern_frequency_hz(&pattern);
        if (current_frequency_hz != 0U) {
            if (current_frequency_hz != previous_frequency_hz && count < 3U) {
                frequency_hz[count] = current_frequency_hz;
                duty_percent[count] = sp01_buzzer_pattern_duty_percent(&pattern);
                count++;
            }
            if (count > 0U && current_frequency_hz == frequency_hz[count - 1U]) {
                tone_ticks[count - 1U]++;
            }
        }
        previous_frequency_hz = current_frequency_hz;
    }
    assert(count == 3U);
    assert(frequency_hz[0] == 2000U);
    assert(frequency_hz[1] == 2500U);
    assert(frequency_hz[2] == 3000U);
    assert(duty_percent[0] == SP01_POWER_ON_DUTY_1_PERCENT);
    assert(duty_percent[1] == SP01_POWER_ON_DUTY_2_PERCENT);
    assert(duty_percent[2] == SP01_POWER_ON_DUTY_3_PERCENT);
    assert(tone_ticks[0] == SP01_FULL_TEST_BEEP_POWER_ON_1_MS / SP01_TICK_MS);
    assert(tone_ticks[1] == SP01_FULL_TEST_BEEP_POWER_ON_1_MS / SP01_TICK_MS);
    assert(tone_ticks[2] == SP01_FULL_TEST_BEEP_POWER_ON_2_MS / SP01_TICK_MS);

    sp01_buzzer_pattern_init(&pattern);
    count = 0U;
    previous_frequency_hz = 0U;
    tone_ticks[0] = 0U;
    tone_ticks[1] = 0U;
    tone_ticks[2] = 0U;
    for (i = 0U; i < 42U; i++) {
        (void)sp01_buzzer_pattern_update(&pattern, SP01_BUZZER_POWER_OFF);
        current_frequency_hz = sp01_buzzer_pattern_frequency_hz(&pattern);
        if (current_frequency_hz != 0U) {
            if (current_frequency_hz != previous_frequency_hz && count < 3U) {
                frequency_hz[count] = current_frequency_hz;
                duty_percent[count] = sp01_buzzer_pattern_duty_percent(&pattern);
                count++;
            }
            if (count > 0U && current_frequency_hz == frequency_hz[count - 1U]) {
                tone_ticks[count - 1U]++;
            }
        }
        previous_frequency_hz = current_frequency_hz;
    }
    assert(count == 3U);
    assert(frequency_hz[0] == 3000U);
    assert(frequency_hz[1] == 2500U);
    assert(frequency_hz[2] == 2000U);
    assert(duty_percent[0] == SP01_POWER_OFF_DUTY_1_PERCENT);
    assert(duty_percent[1] == SP01_POWER_OFF_DUTY_2_PERCENT);
    assert(duty_percent[2] == SP01_POWER_OFF_DUTY_3_PERCENT);
    assert(tone_ticks[0] == SP01_FULL_TEST_BEEP_POWER_OFF_1_MS / SP01_TICK_MS);
    assert(tone_ticks[1] == SP01_FULL_TEST_BEEP_POWER_OFF_1_MS / SP01_TICK_MS);
    assert(tone_ticks[2] == SP01_FULL_TEST_BEEP_POWER_OFF_2_MS / SP01_TICK_MS);

    sp01_buzzer_pattern_init(&pattern);
    (void)sp01_buzzer_pattern_update(&pattern, SP01_BUZZER_KEY_CLICK);
    assert(sp01_buzzer_pattern_duty_percent(&pattern) ==
           SP01_BUZZER_DEFAULT_DUTY_PERCENT);
}

static void test_signal_filter_requires_stable_ticks(void)
{
    sp01_bool_filter_t filter;

    sp01_bool_filter_init(&filter, APP_FALSE);
    assert(sp01_bool_filter_update(&filter, APP_TRUE, 3U) == APP_FALSE);
    assert(sp01_bool_filter_update(&filter, APP_TRUE, 3U) == APP_FALSE);
    assert(sp01_bool_filter_update(&filter, APP_TRUE, 3U) == APP_TRUE);
    assert(sp01_bool_filter_update(&filter, APP_FALSE, 3U) == APP_TRUE);
    assert(sp01_bool_filter_update(&filter, APP_FALSE, 3U) == APP_TRUE);
    assert(sp01_bool_filter_update(&filter, APP_FALSE, 3U) == APP_FALSE);

    /* required_ticks 为 0/1 时应立即采用当前采样。 */
    assert(sp01_bool_filter_update(&filter, APP_TRUE, 1U) == APP_TRUE);
}

static void test_radar_input_stability_and_edges(void)
{
    sp01_radar_input_t radar;
    sp01_radar_event_t event;

    sp01_radar_input_init(&radar, APP_FALSE);

    event = sp01_radar_input_update(&radar, APP_TRUE, 3U);
    assert(event.active == APP_FALSE);
    assert(event.rising_edge == APP_FALSE);
    assert(event.falling_edge == APP_FALSE);

    event = sp01_radar_input_update(&radar, APP_TRUE, 3U);
    assert(event.active == APP_FALSE);

    event = sp01_radar_input_update(&radar, APP_TRUE, 3U);
    assert(event.active == APP_TRUE);
    assert(event.rising_edge == APP_TRUE);
    assert(event.falling_edge == APP_FALSE);

    event = sp01_radar_input_update(&radar, APP_TRUE, 3U);
    assert(event.active == APP_TRUE);
    assert(event.rising_edge == APP_FALSE);

    (void)sp01_radar_input_update(&radar, APP_FALSE, 3U);
    (void)sp01_radar_input_update(&radar, APP_FALSE, 3U);
    event = sp01_radar_input_update(&radar, APP_FALSE, 3U);
    assert(event.active == APP_FALSE);
    assert(event.rising_edge == APP_FALSE);
    assert(event.falling_edge == APP_TRUE);
}

static void test_charge_monitor_debounce_and_conflict(void)
{
    sp01_charge_monitor_t monitor;
    sp01_charge_status_t status;

    sp01_charge_monitor_init(&monitor);

    status = sp01_charge_monitor_update(&monitor, APP_TRUE, APP_FALSE, 3U);
    assert(status.charging_active == APP_FALSE);
    assert(status.charge_done == APP_FALSE);
    assert(status.conflict == APP_FALSE);

    (void)sp01_charge_monitor_update(&monitor, APP_TRUE, APP_FALSE, 3U);
    status = sp01_charge_monitor_update(&monitor, APP_TRUE, APP_FALSE, 3U);
    assert(status.charging_active == APP_TRUE);
    assert(status.charge_done == APP_FALSE);

    (void)sp01_charge_monitor_update(&monitor, APP_TRUE, APP_TRUE, 3U);
    (void)sp01_charge_monitor_update(&monitor, APP_TRUE, APP_TRUE, 3U);
    status = sp01_charge_monitor_update(&monitor, APP_TRUE, APP_TRUE, 3U);
    assert(status.charging_active == APP_TRUE);
    assert(status.charge_done == APP_TRUE);
    assert(status.conflict == APP_TRUE);

    (void)sp01_charge_monitor_update(&monitor, APP_FALSE, APP_TRUE, 3U);
    (void)sp01_charge_monitor_update(&monitor, APP_FALSE, APP_TRUE, 3U);
    status = sp01_charge_monitor_update(&monitor, APP_FALSE, APP_TRUE, 3U);
    assert(status.charging_active == APP_FALSE);
    assert(status.charge_done == APP_TRUE);
    assert(status.conflict == APP_FALSE);
}

static void test_touch_button_short_and_long_events(void)
{
    sp01_touch_button_t button;
    sp01_touch_button_event_t event;
    app_u32 i;

    sp01_touch_button_init(&button);

    /* 短按：按下只产生 pressed_edge，松手才产生 short_release。 */
    event = sp01_touch_button_update(&button, APP_TRUE, APP_FALSE, 10UL, 2000UL);
    assert(event.pressed_edge == APP_TRUE);
    assert(event.short_release == APP_FALSE);
    assert(event.long_edge == APP_FALSE);

    event = sp01_touch_button_update(&button, APP_FALSE, APP_FALSE, 10UL, 2000UL);
    assert(event.short_release == APP_TRUE);
    assert(event.long_edge == APP_FALSE);

    /* 长按：达到阈值只报一次 long_edge，松手不再报短按。 */
    sp01_touch_button_init(&button);
    (void)sp01_touch_button_update(&button, APP_TRUE, APP_FALSE, 10UL, 2000UL);
    for (i = 1U; i < 199U; i++) {
        event = sp01_touch_button_update(&button, APP_TRUE, APP_FALSE, 10UL, 2000UL);
        assert(event.long_edge == APP_FALSE);
    }
    event = sp01_touch_button_update(&button, APP_TRUE, APP_FALSE, 10UL, 2000UL);
    assert(event.long_edge == APP_TRUE);
    event = sp01_touch_button_update(&button, APP_TRUE, APP_FALSE, 10UL, 2000UL);
    assert(event.long_edge == APP_FALSE);
    event = sp01_touch_button_update(&button, APP_FALSE, APP_FALSE, 10UL, 2000UL);
    assert(event.short_release == APP_FALSE);
}

static void test_indicator_mapping(void)
{
    sp01_indicator_pins_t pins;

    sp01_indicator_render(SP01_LED_OFF, SP01_BUZZER_OFF, &pins);
    assert(pins.red == APP_FALSE);
    assert(pins.green == APP_FALSE);
    assert(pins.blue == APP_FALSE);
    assert(pins.buzzer_on == APP_FALSE);

    sp01_indicator_render(SP01_LED_CHARGING, SP01_BUZZER_KEY_CLICK, &pins);
    assert(pins.red == APP_FALSE);
    assert(pins.green == APP_FALSE);
    assert(pins.blue == APP_TRUE);
    assert(pins.buzzer_on == APP_TRUE);

    sp01_indicator_render(SP01_LED_FULL, SP01_BUZZER_OFF, &pins);
    assert(pins.red == APP_FALSE);
    assert(pins.green == APP_TRUE);
    assert(pins.blue == APP_FALSE);

    sp01_indicator_render(SP01_LED_STANDBY, SP01_BUZZER_OFF, &pins);
    assert(pins.red == APP_FALSE);
    assert(pins.green == APP_FALSE);
    assert(pins.blue == APP_FALSE);

    sp01_indicator_render(SP01_LED_PUMPING, SP01_BUZZER_OFF, &pins);
    assert(pins.red == APP_FALSE);
    assert(pins.green == APP_TRUE);
    assert(pins.blue == APP_TRUE);

    /* FullTest001: 普通 15 秒出水只点亮绿色通道。 */
    sp01_indicator_render(SP01_LED_SENSOR_PUMPING, SP01_BUZZER_OFF, &pins);
    assert(pins.red == APP_FALSE);
    assert(pins.green == APP_TRUE);
    assert(pins.blue == APP_FALSE);

    /* FullTest001: 连续出水只点亮蓝色通道。 */
    sp01_indicator_render(SP01_LED_CONTINUOUS, SP01_BUZZER_OFF, &pins);
    assert(pins.red == APP_FALSE);
    assert(pins.green == APP_FALSE);
    assert(pins.blue == APP_TRUE);

    sp01_indicator_render(SP01_LED_LOW_WATER, SP01_BUZZER_LOW_WATER, &pins);
    assert(pins.red == APP_TRUE);
    assert(pins.green == APP_FALSE);
    assert(pins.blue == APP_FALSE);
    assert(pins.buzzer_on == APP_TRUE);

    sp01_indicator_render(SP01_LED_FAULT, SP01_BUZZER_FAULT, &pins);
    assert(pins.red == APP_TRUE);
    assert(pins.green == APP_FALSE);
    assert(pins.blue == APP_FALSE);
    assert(pins.buzzer_on == APP_TRUE);

    /* SPEC_TEST007：Type-C 低水位预警白灯，电池预警绿灯。 */
    sp01_indicator_render(SP01_LED_PRODUCT_LOW_WATER_WARNING_TYPEC,
                          SP01_BUZZER_OFF,
                          &pins);
    assert(pins.red == APP_TRUE);
    assert(pins.green == APP_TRUE);
    assert(pins.blue == APP_TRUE);

    sp01_indicator_render(SP01_LED_PRODUCT_LOW_WATER_WARNING_BATTERY,
                          SP01_BUZZER_OFF,
                          &pins);
    assert(pins.red == APP_FALSE);
    assert(pins.green == APP_TRUE);
    assert(pins.blue == APP_FALSE);
}

static void test_software_end_to_end_normal_run(void)
{
    sim_runtime_t runtime;
    sp01_outputs_t outputs;
    app_u32 i;

    sim_runtime_init(&runtime);

    for (i = 0U; i < (SP01_BOOT_SETTLE_MS / SP01_TICK_MS) + 1U; i++) {
        outputs = sim_runtime_tick(&runtime, APP_FALSE, 0U);
    }
    assert(runtime.app.state == SP01_STATE_STANDBY);

    for (i = 0U; i < SP01_RADAR_DEBOUNCE_TICKS; i++) {
        outputs = sim_runtime_tick(&runtime, APP_TRUE, 0U);
    }
    assert(runtime.app.state == SP01_STATE_SENSOR_RUN);
    assert(outputs.pump_enable == APP_FALSE);

    outputs = sim_runtime_tick(&runtime, APP_FALSE, 180U);
    assert(outputs.pump_enable == APP_TRUE);

    for (i = 0U; i < SP01_SENSOR_RUN_MS / SP01_TICK_MS; i++) {
        outputs = sim_runtime_tick(&runtime, APP_FALSE, 180U);
    }
    assert(runtime.app.state == SP01_STATE_LOCKOUT);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.radar_power_enable == APP_TRUE);
}

static void test_software_end_to_end_low_water_alarm(void)
{
    sim_runtime_t runtime;
    sp01_outputs_t outputs;
    app_u32 i;

    sim_runtime_init(&runtime);
    for (i = 0U; i < (SP01_BOOT_SETTLE_MS / SP01_TICK_MS) + 1U; i++) {
        (void)sim_runtime_tick(&runtime, APP_FALSE, 0U);
    }
    for (i = 0U; i < SP01_RADAR_DEBOUNCE_TICKS; i++) {
        (void)sim_runtime_tick(&runtime, APP_TRUE, 0U);
    }
    (void)sim_runtime_tick(&runtime, APP_FALSE, 100U);

    for (i = 0U; i < (SP01_PUMP_START_IGNORE_MS + SP01_PUMP_DIAG_WINDOW_MS) /
                      SP01_TICK_MS; i++) {
        outputs = sim_runtime_tick(&runtime, APP_FALSE, 100U);
    }
    assert(runtime.app.state == SP01_STATE_LOW_WATER_SUSPECT);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.led_mode == SP01_LED_LOW_WATER);

    for (i = 0U; i < SP01_LOW_WATER_RETRY_OFF_MS / SP01_TICK_MS; i++) {
        (void)sim_runtime_tick(&runtime, APP_FALSE, 100U);
    }
    assert(runtime.app.state == SP01_STATE_SENSOR_RUN);

    /* 第二次连续诊断仍为低水位，才进入红灯报警锁定。 */
    (void)sim_runtime_tick(&runtime, APP_FALSE, 100U);
    for (i = 0U; i < (SP01_PUMP_START_IGNORE_MS + SP01_PUMP_DIAG_WINDOW_MS) /
                      SP01_TICK_MS; i++) {
        outputs = sim_runtime_tick(&runtime, APP_FALSE, 100U);
    }
    assert(runtime.app.state == SP01_STATE_LOW_WATER_ALARM);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.led_mode == SP01_LED_LOW_WATER);
}

static void test_board_outputs_fail_safe_and_tick(void)
{
    board_inputs_t inputs;
    sp01_outputs_t outputs;
    sp01_outputs_t observed;

    board_init();
    observed = board_sim_get_outputs();
    assert(observed.pump_enable == APP_FALSE);
    assert(observed.radar_power_enable == APP_FALSE);
    assert(observed.led_mode == SP01_LED_OFF);
    assert(observed.buzzer_mode == SP01_BUZZER_OFF);

    outputs.pump_enable = APP_TRUE;
    outputs.radar_power_enable = APP_TRUE;
    outputs.led_mode = SP01_LED_PUMPING;
    outputs.buzzer_mode = SP01_BUZZER_OFF;
    board_apply_outputs(&outputs);
    observed = board_sim_get_outputs();
    assert(observed.pump_enable == APP_TRUE);
    assert(observed.radar_power_enable == APP_TRUE);
    assert(observed.led_mode == SP01_LED_PUMPING);

    inputs.radar_active = APP_TRUE;
    inputs.key_pressed = APP_FALSE;
    inputs.key_long_pressed = APP_FALSE;
    inputs.charging_active = APP_FALSE;
    inputs.charge_done = APP_FALSE;
    inputs.pump_sense_mv = 180U;
    inputs.battery_mv = 3800U;
    inputs.ntc_mv = 1200U;
    board_sim_set_inputs(inputs);
    assert(board_read_inputs().radar_active == APP_TRUE);
    assert(board_read_inputs().pump_sense_mv == 180U);

    assert(board_take_10ms_tick() == APP_FALSE);
    board_sim_force_tick();
    assert(board_take_10ms_tick() == APP_TRUE);
    assert(board_take_10ms_tick() == APP_FALSE);
}

static void test_long_press_toggles_night_mode(void)
{
    sp01_app_t app;
    sp01_inputs_t inputs;
    app_u32 i;

    sp01_app_init(&app);
    inputs = make_inputs();
    tick_app(&app, &inputs, (SP01_BOOT_SETTLE_MS / SP01_TICK_MS) + 1U);

    inputs.key_pressed = APP_TRUE;
    sp01_app_tick(&app, &inputs);
    assert(app.night_mode_enabled == APP_FALSE);
    assert(app.state == SP01_STATE_STANDBY);

    for (i = 0U; i < (SP01_KEY_LONG_PRESS_MS / SP01_TICK_MS); i++) {
        sp01_app_tick(&app, &inputs);
    }
    assert(app.night_mode_enabled == APP_TRUE);

    /* 松手后再次短按不能误触发第二次切换。 */
    inputs.key_pressed = APP_FALSE;
    sp01_app_tick(&app, &inputs);
    inputs.key_pressed = APP_TRUE;
    sp01_app_tick(&app, &inputs);
    assert(app.night_mode_enabled == APP_TRUE);
    assert(app.state == SP01_STATE_STANDBY);
}

static void test_fault_hold_does_not_toggle_night_mode(void)
{
    sp01_app_t app;
    sp01_inputs_t inputs;

    sp01_app_init(&app);
    inputs = make_inputs();
    tick_app(&app, &inputs, (SP01_BOOT_SETTLE_MS / SP01_TICK_MS) + 1U);

    inputs.radar_active = APP_TRUE;
    (void)sp01_app_tick(&app, &inputs);
    inputs.pump_diag = SP01_PUMP_DIAG_OVER_CURRENT;
    (void)sp01_app_tick(&app, &inputs);
    assert(app.state == SP01_STATE_FAULT);
    assert(app.night_mode_enabled == APP_FALSE);

    inputs.radar_active = APP_FALSE;
    inputs.pump_diag = SP01_PUMP_DIAG_UNKNOWN;
    inputs.key_pressed = APP_TRUE;
    (void)sp01_app_tick(&app, &inputs);
    (void)tick_app(&app,
                   &inputs,
                   SP01_KEY_LONG_PRESS_MS / SP01_TICK_MS);

    assert(app.state == SP01_STATE_STANDBY);
    assert(app.night_mode_enabled == APP_FALSE);
}

static void test_over_current_enters_fault_and_disables_outputs(void)
{
    sp01_app_t app;
    sp01_inputs_t inputs;
    sp01_outputs_t outputs;

    sp01_app_init(&app);
    inputs = make_inputs();
    tick_app(&app, &inputs, (SP01_BOOT_SETTLE_MS / SP01_TICK_MS) + 1U);

    inputs.radar_active = APP_TRUE;
    sp01_app_tick(&app, &inputs);
    inputs.radar_active = APP_FALSE;
    inputs.pump_diag = SP01_PUMP_DIAG_OVER_CURRENT;

    outputs = sp01_app_tick(&app, &inputs);
    assert(app.state == SP01_STATE_FAULT);
    assert(app.fault_code == SP01_FAULT_PUMP_OVER_CURRENT);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.radar_power_enable == APP_FALSE);
    assert(outputs.led_mode == SP01_LED_FAULT);

    inputs.key_pressed = APP_TRUE;
    outputs = sp01_app_tick(&app, &inputs);
    assert(app.state == SP01_STATE_FAULT);
    assert(outputs.pump_enable == APP_FALSE);

    inputs.key_pressed = APP_FALSE;
    outputs = sp01_app_tick(&app, &inputs);
    assert(app.state == SP01_STATE_FAULT);

    inputs.key_pressed = APP_TRUE;
    outputs = sp01_app_tick(&app, &inputs);
    assert(app.state == SP01_STATE_FAULT);

    outputs = tick_app(&app,
                       &inputs,
                       (SP01_KEY_LONG_PRESS_MS / SP01_TICK_MS) - 1U);
    assert(app.state == SP01_STATE_STANDBY);
    assert(app.fault_code == SP01_FAULT_NONE);
    assert(outputs.pump_enable == APP_FALSE);
}

static void test_fault_acknowledgement_is_blocked_while_battery_is_low(void)
{
    sp01_app_t app;
    sp01_inputs_t inputs;

    sp01_app_init(&app);
    inputs = make_inputs();
    tick_app(&app, &inputs, (SP01_BOOT_SETTLE_MS / SP01_TICK_MS) + 1U);

    inputs.radar_active = APP_TRUE;
    (void)sp01_app_tick(&app, &inputs);
    inputs.pump_diag = SP01_PUMP_DIAG_OVER_CURRENT;
    (void)sp01_app_tick(&app, &inputs);
    assert(app.state == SP01_STATE_FAULT);

    inputs.radar_active = APP_FALSE;
    inputs.pump_diag = SP01_PUMP_DIAG_UNKNOWN;
    inputs.key_pressed = APP_TRUE;
    (void)sp01_app_tick(&app, &inputs);
    inputs.battery_low = APP_TRUE;
    (void)tick_app(&app,
                   &inputs,
                   (SP01_KEY_LONG_PRESS_MS / SP01_TICK_MS) - 1U);
    assert(app.state == SP01_STATE_FAULT);
    assert(app.fault_code == SP01_FAULT_PUMP_OVER_CURRENT);
}

static void test_battery_low_fault_code_clears_after_recovery(void)
{
    sp01_app_t app;
    sp01_inputs_t inputs;
    sp01_outputs_t outputs;

    sp01_app_init(&app);
    inputs = make_inputs();
    tick_app(&app, &inputs, (SP01_BOOT_SETTLE_MS / SP01_TICK_MS) + 1U);

    inputs.battery_low = APP_TRUE;
    outputs = sp01_app_tick(&app, &inputs);
    assert(app.state == SP01_STATE_STANDBY);
    assert(app.fault_code == SP01_FAULT_BATTERY_LOW);
    assert(outputs.led_mode == SP01_LED_FAULT);

    inputs.battery_low = APP_FALSE;
    outputs = sp01_app_tick(&app, &inputs);
    assert(app.fault_code == SP01_FAULT_NONE);
    assert(outputs.led_mode == SP01_LED_STANDBY);
}

static void test_battery_low_stops_active_pump_immediately(void)
{
    sp01_app_t app;
    sp01_inputs_t inputs;
    sp01_outputs_t outputs;

    sp01_app_init(&app);
    inputs = make_inputs();
    tick_app(&app, &inputs, (SP01_BOOT_SETTLE_MS / SP01_TICK_MS) + 1U);

    inputs.radar_active = APP_TRUE;
    (void)sp01_app_tick(&app, &inputs);
    inputs.radar_active = APP_FALSE;
    outputs = sp01_app_tick(&app, &inputs);
    assert(app.state == SP01_STATE_SENSOR_RUN);
    assert(outputs.pump_enable == APP_TRUE);

    inputs.battery_low = APP_TRUE;
    outputs = sp01_app_tick(&app, &inputs);
    assert(app.state == SP01_STATE_STANDBY);
    assert(app.fault_code == SP01_FAULT_BATTERY_LOW);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.led_mode == SP01_LED_FAULT);
}

static void test_manual_run_has_time_limit(void)
{
    sp01_app_t app;
    sp01_inputs_t inputs;
    sp01_outputs_t outputs;

    sp01_app_init(&app);
    inputs = make_inputs();
    tick_app(&app, &inputs, (SP01_BOOT_SETTLE_MS / SP01_TICK_MS) + 1U);

    inputs.key_pressed = APP_TRUE;
    outputs = sp01_app_tick(&app, &inputs);
    assert(app.state == SP01_STATE_STANDBY);
    assert(outputs.pump_enable == APP_FALSE);

    inputs.key_pressed = APP_FALSE;
    outputs = sp01_app_tick(&app, &inputs);
    assert(app.state == SP01_STATE_MANUAL_RUN);
    assert(outputs.pump_enable == APP_FALSE);

    outputs = sp01_app_tick(&app, &inputs);
    assert(outputs.pump_enable == APP_TRUE);

    outputs = tick_app(&app, &inputs, SP01_MANUAL_RUN_MAX_MS / SP01_TICK_MS);
    assert(app.state == SP01_STATE_STANDBY);
    assert(outputs.pump_enable == APP_FALSE);
}

int main(void)
{
    test_boot_defaults_safe();
    test_radar_run_then_lockout();
    test_radar_must_return_idle_after_boot_before_starting_pump();
    test_low_water_requires_two_failed_checks();
    test_pump_monitor_classification();
    test_power_monitor_classification_and_debounce();
    test_buzzer_pattern_is_non_blocking();
    test_power_buzzer_duty_matches_spec();
    test_signal_filter_requires_stable_ticks();
    test_radar_input_stability_and_edges();
    test_charge_monitor_debounce_and_conflict();
    test_touch_button_short_and_long_events();
    test_indicator_mapping();
    test_software_end_to_end_normal_run();
    test_software_end_to_end_low_water_alarm();
    test_board_outputs_fail_safe_and_tick();
    test_long_press_toggles_night_mode();
    test_fault_hold_does_not_toggle_night_mode();
    test_over_current_enters_fault_and_disables_outputs();
    test_fault_acknowledgement_is_blocked_while_battery_is_low();
    test_battery_low_fault_code_clears_after_recovery();
    test_battery_low_stops_active_pump_immediately();
    test_manual_run_has_time_limit();

    puts("SP01 host tests passed");
    return 0;
}
