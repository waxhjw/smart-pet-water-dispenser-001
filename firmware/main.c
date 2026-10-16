#include "app/app_config.h"
#include "app/sp01_build_info.h"
#include "app/sp01_app.h"
#include "app/sp01_buzzer_pattern.h"
#include "app/sp01_power_monitor.h"
#include "app/sp01_pump_monitor.h"
#include "app/sp01_signal_filter.h"
#include "app/sp01_radar_input.h"
#include "app/sp01_charge_monitor.h"
#include "app/sp01_battery_test.h"
#include "app/sp01_battery_pump_test.h"
#include "app/sp01_pump_adc_test.h"
#include "app/sp01_pump_adc_hold_test.h"
#include "app/sp01_full_test.h"
#include "app/sp01_spec_product.h"
#include "app/sp01_rgb_test.h"
#include "app/sp01_touch_diag_test.h"
#include "platform/py32f002b/board_port.h"

static sp01_app_t g_app;
static sp01_buzzer_pattern_t g_buzzer_pattern;
static sp01_power_monitor_t g_power_monitor;
static sp01_pump_monitor_t g_pump_monitor;
static sp01_outputs_t g_last_outputs;
static sp01_radar_input_t g_radar_input;
static sp01_bool_filter_t g_key_filter;
static sp01_charge_monitor_t g_charge_monitor;
#ifdef SP01_TOUCH_DIAG_TEST
static sp01_touch_diag_test_t g_touch_diag_test;
#endif
#ifdef SP01_BATTERY_PUMP_TEST
static sp01_battery_pump_test_t g_battery_pump_test;
#endif
#ifdef SP01_PUMP_ADC_TEST
static sp01_pump_adc_test_t g_pump_adc_test;
#endif
#ifdef SP01_PUMP_ADC_HOLD_TEST
static sp01_pump_adc_hold_test_t g_pump_adc_hold_test;
#endif
#ifdef SP01_FULL_TEST
static sp01_full_test_t g_full_test;
#endif
#ifdef SP01_SPEC_TEST
static sp01_spec_product_t g_spec_product;
#endif
#if defined(SP01_RGB_TUNER_TEST) || defined(SP01_RGB_TEST) || defined(SP01_RGB_SINGLE_TEST) || \
    defined(SP01_RGB_ISOLATED_PA5_TEST) || defined(SP01_RGB_ISOLATED_PB3_TEST) || \
    defined(SP01_RGB_COMBO_PA5_PA1_TEST) || defined(SP01_RGB_COMBO_SCAN_TEST) || \
    defined(SP01_RGB_ALL_ON_TEST) || defined(SP01_RGB_WHITE_BALANCE_TEST)
static sp01_rgb_test_t g_rgb_test;
#endif
#ifdef SP01_RADAR_TEST
static app_u32 g_radar_test_elapsed_ms;
static app_u32 g_radar_test_idle_ms;
static app_bool_t g_radar_test_armed;
#endif
/*
 * 首板时通过 SWD 查看版本和构建模式，避免样机、HEX 和源码版本对不上。
 * 这些变量不参与产品流程，只用于现场确认。
 */
volatile const app_u32 g_sp01_debug_fw_version = SP01_FW_VERSION_CODE;
#ifdef SP01_TOUCH_DIAG_TEST
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_TOUCH_DIAG_TEST;
#elif defined(SP01_RELEASE_BUILD)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_RELEASE;
#elif defined(SP01_RADAR_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_RADAR_TEST;
#elif defined(SP01_PUMP_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_PUMP_TEST;
#elif defined(SP01_BATTERY_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_BATTERY_TEST;
#elif defined(SP01_BATTERY_PUMP_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_BATTERY_PUMP_TEST;
#elif defined(SP01_PUMP_ADC_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_PUMP_ADC_TEST;
#elif defined(SP01_PUMP_ADC_HOLD_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_PUMP_ADC_HOLD_TEST;
#elif defined(SP01_LOW_WATER_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_LOW_WATER_TEST;
#elif defined(SP01_FULL_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_FULL_TEST;
#elif defined(SP01_SPEC_TEST005)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_SPEC_TEST005;
#elif defined(SP01_SPEC_TEST006)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_SPEC_TEST006;
#elif defined(SP01_SPEC_TEST007)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_SPEC_TEST007;
#elif defined(SP01_SPEC_TEST003)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_SPEC_TEST003;
#elif defined(SP01_SPEC_TEST004)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_SPEC_TEST004;
#elif defined(SP01_SPEC_TEST002)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_SPEC_TEST002;
#elif defined(SP01_SPEC_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_SPEC_TEST;
#elif defined(SP01_RGB_SINGLE_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_RGB_SINGLE_TEST;
#elif defined(SP01_RGB_ISOLATED_PA5_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_RGB_ISOLATED_PA5_TEST;
#elif defined(SP01_RGB_ISOLATED_PB3_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_RGB_ISOLATED_PB3_TEST;
#elif defined(SP01_RGB_COMBO_PA5_PA1_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_RGB_COMBO_PA5_PA1_TEST;
#elif defined(SP01_RGB_COMBO_SCAN_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_RGB_COMBO_SCAN_TEST;
#elif defined(SP01_RGB_ALL_ON_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_RGB_ALL_ON_TEST;
#elif defined(SP01_RGB_WHITE_BALANCE_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_RGB_WHITE_BALANCE_TEST;
#elif defined(SP01_RGB_TUNER_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_RGB_TUNER_TEST;
#elif defined(SP01_RGB_TEST)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_RGB_TEST;
#elif defined(SP01_BENCH_NO_PUMP)
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_BENCH_NO_PUMP;
#else
volatile const app_u8 g_sp01_debug_build_profile = SP01_BUILD_PROFILE_DEBUG;
#endif

/*
 * 首板调试观察点：通过 SWD 查看这些全局变量即可看到最近一次采样和决策，
 * 不依赖串口，也不改变产品流程。量产版不保留这组快照变量。
 */
#ifndef SP01_RELEASE_BUILD
volatile board_inputs_t g_sp01_debug_inputs;
volatile board_inputs_t g_sp01_debug_raw_inputs;
volatile sp01_outputs_t g_sp01_debug_outputs;
volatile sp01_outputs_t g_sp01_debug_applied_outputs;
volatile sp01_power_snapshot_t g_sp01_debug_power_snapshot;
volatile sp01_pump_snapshot_t g_sp01_debug_pump_snapshot;
volatile sp01_radar_event_t g_sp01_debug_radar_event;
volatile sp01_charge_status_t g_sp01_debug_charge_status;
volatile sp01_state_t g_sp01_debug_state;
volatile sp01_fault_code_t g_sp01_debug_fault;
volatile board_reset_reason_t g_sp01_debug_reset_reason;
/* ADC 外部通道扫描快照：上位机通过 SWD 读取，通道对应的实际引脚需结合网表确认。 */
volatile app_u16 g_sp01_debug_adc_scan_mv[8];
volatile app_u16 g_sp01_debug_adc_scan_raw[8];
volatile app_u32 g_sp01_debug_adc_scan_seq;
volatile app_u8 g_sp01_debug_adc_selected_channel;
volatile app_u16 g_sp01_debug_adc_selected_raw;
volatile app_u16 g_sp01_debug_adc_selected_mv;
volatile app_u16 g_sp01_debug_adc_selected_average_raw;
volatile app_u16 g_sp01_debug_adc_selected_average_mv;
volatile app_u16 g_sp01_debug_adc_sample_count;
volatile app_u16 g_sp01_debug_adc_discard_count;
volatile app_bool_t g_sp01_debug_adc_stable;
#ifdef SP01_FULL_TEST
volatile sp01_full_power_state_t g_sp01_debug_full_power_state;
volatile app_bool_t g_sp01_debug_full_night_mode;
volatile app_u32 g_sp01_debug_full_radar_cooldown_ms;
#endif
#ifdef SP01_SPEC_TEST
volatile sp01_spec_state_t g_sp01_debug_spec_state;
volatile sp01_spec_supply_t g_sp01_debug_spec_supply;
volatile app_bool_t g_sp01_debug_spec_night_mode;
#endif
/* 设为 0 后继续运行，可验证 IWDG 是否会在约 2 秒后复位。 */
volatile app_bool_t g_sp01_debug_watchdog_refresh_enabled = APP_TRUE;
volatile app_u32 g_sp01_debug_uptime_ms;
#endif

#ifdef SP01_PUMP_TEST
static sp01_outputs_t sp01_pump_test_outputs(void)
{
    sp01_outputs_t outputs;

    /* 用户确认的持续供电测试：PB2 常高，无按键启动或自动关断。 */
    outputs.pump_enable = APP_TRUE;
    outputs.radar_power_enable = APP_FALSE;
    outputs.led_mode = SP01_LED_OFF;
    outputs.buzzer_mode = SP01_BUZZER_OFF;
    outputs.buzzer_frequency_hz = 0U;
    outputs.buzzer_duty_percent = 0U;
    return outputs;
}
#endif

static void sp01_filter_board_inputs(board_inputs_t *inputs,
                                     sp01_radar_event_t *radar_event_out,
                                     sp01_charge_status_t *charge_status_out)
{
    sp01_radar_event_t radar_event;
    sp01_charge_status_t charge_status;

    radar_event = sp01_radar_input_update(&g_radar_input,
                                          inputs->radar_active,
                                          SP01_RADAR_DEBOUNCE_TICKS);
    charge_status = sp01_charge_monitor_update(&g_charge_monitor,
                                                inputs->charging_active,
                                                inputs->charge_done,
                                                SP01_CHARGE_DEBOUNCE_TICKS);

    inputs->radar_active = radar_event.active;
#ifndef SP01_TOUCH_DIAG_TEST
    inputs->key_pressed = sp01_bool_filter_update(&g_key_filter,
                                                  inputs->key_pressed,
                                                  SP01_KEY_DEBOUNCE_TICKS);
#endif
    inputs->charging_active = charge_status.charging_active;
    inputs->charge_done = charge_status.charge_done;

    if (radar_event_out != 0) {
        *radar_event_out = radar_event;
    }
    if (charge_status_out != 0) {
        *charge_status_out = charge_status;
    }
}

#if !defined(SP01_FULL_TEST) && !defined(SP01_SPEC_TEST) && \
    !defined(SP01_TOUCH_DIAG_TEST)
static sp01_inputs_t app_inputs_from_board(const board_inputs_t *board,
                                           sp01_power_diag_t power_diag,
                                           sp01_pump_diag_t pump_diag)
{
    sp01_inputs_t inputs;

    inputs.radar_active = board->radar_active;
    inputs.key_pressed = board->key_pressed;
    inputs.key_long_pressed = board->key_long_pressed;
    inputs.charging_active = board->charging_active;
    inputs.charge_done = board->charge_done;
#if defined(SP01_BENCH_NO_PUMP) || defined(SP01_PUMP_TEST) || \
    defined(SP01_BATTERY_TEST) || defined(SP01_BATTERY_PUMP_TEST)
    /* 台架版只屏蔽电池欠压对流程的阻断；原始采样和诊断仍保留供调试观察。 */
    (void)power_diag;
    inputs.battery_low = APP_FALSE;
#else
    inputs.battery_low = (power_diag == SP01_POWER_DIAG_LOW ||
                          power_diag == SP01_POWER_DIAG_CRITICAL ||
                          power_diag == SP01_POWER_DIAG_INVALID) ? APP_TRUE : APP_FALSE;
#endif
    inputs.pump_diag = pump_diag;

    return inputs;
}
#endif

static void sp01_system_init(void)
{
    app_u8 channel;

    board_init();
    sp01_app_init(&g_app);
    sp01_buzzer_pattern_init(&g_buzzer_pattern);
    sp01_power_monitor_init(&g_power_monitor);
    sp01_pump_monitor_init(&g_pump_monitor);
    sp01_radar_input_init(&g_radar_input, APP_FALSE);
    sp01_bool_filter_init(&g_key_filter, APP_FALSE);
    sp01_charge_monitor_init(&g_charge_monitor);
#ifdef SP01_TOUCH_DIAG_TEST
    sp01_touch_diag_test_init(&g_touch_diag_test);
#endif
#ifdef SP01_BATTERY_PUMP_TEST
    sp01_battery_pump_test_init(&g_battery_pump_test);
#endif
#ifdef SP01_PUMP_ADC_TEST
    sp01_pump_adc_test_init(&g_pump_adc_test);
#endif
#ifdef SP01_PUMP_ADC_HOLD_TEST
    sp01_pump_adc_hold_test_init(&g_pump_adc_hold_test);
#endif
#ifdef SP01_FULL_TEST
    sp01_full_test_init(&g_full_test);
#endif
#ifdef SP01_SPEC_TEST
    sp01_spec_product_init(&g_spec_product);
#endif
#if defined(SP01_RGB_TUNER_TEST) || defined(SP01_RGB_TEST) || defined(SP01_RGB_SINGLE_TEST) || \
    defined(SP01_RGB_ISOLATED_PA5_TEST) || defined(SP01_RGB_ISOLATED_PB3_TEST) || \
    defined(SP01_RGB_COMBO_PA5_PA1_TEST) || defined(SP01_RGB_COMBO_SCAN_TEST) || \
    defined(SP01_RGB_ALL_ON_TEST) || defined(SP01_RGB_WHITE_BALANCE_TEST)
    sp01_rgb_test_init(&g_rgb_test);
#endif
#ifdef SP01_RADAR_TEST
    g_radar_test_elapsed_ms = 0UL;
    g_radar_test_idle_ms = 0UL;
    g_radar_test_armed = APP_FALSE;
#endif
    board_safe_outputs_off();
    g_last_outputs.pump_enable = APP_FALSE;
    g_last_outputs.radar_power_enable = APP_FALSE;
    g_last_outputs.led_mode = SP01_LED_OFF;
    g_last_outputs.buzzer_mode = SP01_BUZZER_OFF;
    g_last_outputs.buzzer_frequency_hz = 0U;
    g_last_outputs.buzzer_duty_percent = 0U;
#ifdef SP01_PUMP_TEST
    g_last_outputs = sp01_pump_test_outputs();
    board_apply_outputs(&g_last_outputs);
#endif
#ifdef SP01_BATTERY_PUMP_TEST
    /* 电池常高测试版从初始化结束就保持 PB2 高电平，便于随时测量。 */
    g_last_outputs.pump_enable = APP_TRUE;
    g_last_outputs.radar_power_enable = APP_FALSE;
    g_last_outputs.led_mode = SP01_LED_OFF;
    g_last_outputs.buzzer_mode = SP01_BUZZER_OFF;
    board_apply_outputs(&g_last_outputs);
#endif
#ifndef SP01_RELEASE_BUILD
    g_sp01_debug_reset_reason = board_get_reset_reason();
    g_sp01_debug_uptime_ms = 0UL;
    g_sp01_debug_adc_selected_channel = (app_u8)SP01_PUMP_ADC_CHANNEL;
    g_sp01_debug_adc_selected_raw = 0U;
    g_sp01_debug_adc_selected_mv = 0U;
    g_sp01_debug_adc_selected_average_raw = 0U;
    g_sp01_debug_adc_selected_average_mv = 0U;
    g_sp01_debug_adc_sample_count = 0U;
    g_sp01_debug_adc_discard_count = 0U;
    g_sp01_debug_adc_stable = APP_FALSE;
    for (channel = 0U; channel < 8U; channel++) {
        g_sp01_debug_adc_scan_mv[channel] = 0U;
        g_sp01_debug_adc_scan_raw[channel] = 0U;
    }
    g_sp01_debug_raw_inputs = board_read_inputs();
    g_sp01_debug_inputs = g_sp01_debug_raw_inputs;
    g_sp01_debug_power_snapshot = sp01_power_monitor_snapshot(&g_power_monitor);
    g_sp01_debug_outputs = g_last_outputs;
    g_sp01_debug_applied_outputs = g_last_outputs;
    g_sp01_debug_pump_snapshot = sp01_pump_monitor_snapshot(&g_pump_monitor,
                                                            g_sp01_debug_inputs.pump_sense_mv);
    g_sp01_debug_radar_event.active = g_sp01_debug_inputs.radar_active;
    g_sp01_debug_radar_event.rising_edge = APP_FALSE;
    g_sp01_debug_radar_event.falling_edge = APP_FALSE;
    g_sp01_debug_charge_status.charging_active = g_sp01_debug_inputs.charging_active;
    g_sp01_debug_charge_status.charge_done = g_sp01_debug_inputs.charge_done;
    g_sp01_debug_charge_status.conflict = APP_FALSE;
    g_sp01_debug_state = g_app.state;
    g_sp01_debug_fault = g_app.fault_code;
#ifdef SP01_FULL_TEST
    g_sp01_debug_full_power_state = g_full_test.power_state;
    g_sp01_debug_full_night_mode = g_full_test.night_mode_enabled;
    g_sp01_debug_full_radar_cooldown_ms = g_full_test.radar_cooldown_ms;
#endif
#endif
}

#if !defined(HOST_SIM) && !defined(SP01_RELEASE_BUILD) && \
    !defined(SP01_SPEC_TEST) && !defined(SP01_TOUCH_DIAG_TEST)
static void sp01_debug_update_adc_scan(void)
{
    app_u8 channel;
    app_u8 sample;
    app_u32 sum_raw;
    app_u16 raw;
    app_u16 stable_raw;
    static app_u32 selected_sum_raw;
    static app_u16 selected_window_count;

    for (channel = 0U; channel < 8U; channel++) {
        /* 每个通道丢弃首次转换，再取 4 次平均，避免通道切换残留。 */
        (void)board_adc_read_channel_raw(channel);
        sum_raw = 0UL;
        for (sample = 0U; sample < SP01_ADC_STABLE_SAMPLE_COUNT; sample++) {
            sum_raw += board_adc_read_channel_raw(channel);
        }
        stable_raw = (app_u16)((sum_raw +
                                (SP01_ADC_STABLE_SAMPLE_COUNT / 2U)) /
                               SP01_ADC_STABLE_SAMPLE_COUNT);
        g_sp01_debug_adc_scan_raw[channel] = stable_raw;
        g_sp01_debug_adc_scan_mv[channel] =
            (app_u16)(((app_u32)stable_raw * 3300UL + 2047UL) / 4095UL);

        if (channel == (app_u8)SP01_PUMP_ADC_CHANNEL) {
            raw = stable_raw;
            g_sp01_debug_adc_selected_raw = raw;
            g_sp01_debug_adc_selected_mv = g_sp01_debug_adc_scan_mv[channel];
            g_sp01_debug_adc_sample_count++;
            g_sp01_debug_adc_discard_count++;
            selected_sum_raw += raw;
            selected_window_count++;
            if (selected_window_count >= SP01_ADC_AVERAGE_WINDOW) {
                g_sp01_debug_adc_selected_average_raw =
                    (app_u16)((selected_sum_raw +
                               (SP01_ADC_AVERAGE_WINDOW / 2U)) /
                              SP01_ADC_AVERAGE_WINDOW);
                g_sp01_debug_adc_selected_average_mv =
                    (app_u16)(((app_u32)g_sp01_debug_adc_selected_average_raw *
                               3300UL + 2047UL) / 4095UL);
                g_sp01_debug_adc_stable = APP_TRUE;
                selected_sum_raw = 0UL;
                selected_window_count = 0U;
            }
        }
    }
    g_sp01_debug_adc_scan_seq++;
}
#endif

static void sp01_system_step(void)
{
    board_inputs_t board_inputs;
#if !defined(SP01_FULL_TEST) && !defined(SP01_SPEC_TEST) && \
    !defined(SP01_TOUCH_DIAG_TEST)
    sp01_inputs_t app_inputs;
#elif defined(SP01_FULL_TEST)
    sp01_full_test_inputs_t full_inputs;
#elif defined(SP01_SPEC_TEST)
    sp01_spec_product_inputs_t spec_inputs;
#endif
    sp01_outputs_t outputs;
    sp01_outputs_t applied_outputs;
#if !defined(SP01_SPEC_TEST) && !defined(SP01_TOUCH_DIAG_TEST)
    sp01_pump_diag_t pump_diag;
#endif
#if !defined(SP01_FULL_TEST) && !defined(SP01_TOUCH_DIAG_TEST)
    sp01_power_diag_t power_diag;
#endif
    sp01_radar_event_t radar_event;
    sp01_charge_status_t charge_status;
    board_inputs = board_read_inputs();
#if !defined(SP01_RELEASE_BUILD) && !defined(HOST_SIM) && \
    !defined(SP01_SPEC_TEST) && !defined(SP01_TOUCH_DIAG_TEST)
    sp01_debug_update_adc_scan();
#endif
#ifndef SP01_RELEASE_BUILD
    g_sp01_debug_raw_inputs = board_inputs;
#endif
    sp01_filter_board_inputs(&board_inputs, &radar_event, &charge_status);

    /*
     * 泵诊断使用上一次应用输出后的真实采样值。
     * 低水位是负载推断，不是真水位传感器，阈值必须打样标定。
     */
#if !defined(SP01_SPEC_TEST) && !defined(SP01_TOUCH_DIAG_TEST)
    pump_diag = sp01_pump_monitor_update(&g_pump_monitor,
                                         g_last_outputs.pump_enable,
                                         board_inputs.pump_sense_mv);
#endif
#if !defined(SP01_FULL_TEST) && !defined(SP01_TOUCH_DIAG_TEST)
    power_diag = sp01_power_monitor_update(&g_power_monitor,
                                           board_inputs.battery_mv);
#elif defined(SP01_FULL_TEST)
    (void)sp01_power_monitor_update(&g_power_monitor, board_inputs.battery_mv);
#endif

#ifdef SP01_TOUCH_DIAG_TEST
    outputs = sp01_touch_diag_test_update(&g_touch_diag_test,
                                          board_inputs.key_pressed);
#elif defined(SP01_FULL_TEST)
    /* Type-C 整机测试专用状态机：不接电池，不调用产品电池保护路径。 */
    full_inputs.key_pressed = board_inputs.key_pressed;
    full_inputs.radar_active = board_inputs.radar_active;
    full_inputs.radar_rising_edge = radar_event.rising_edge;
    full_inputs.pump_diag = pump_diag;
    full_inputs.injected_fault = 0U;
    outputs = sp01_full_test_tick(&g_full_test, &full_inputs);
#elif defined(SP01_SPEC_TEST)
    /* 规格书版使用 4056 的 CHRG/DONE 低有效状态间接识别 Type-C。 */
    spec_inputs.supply = sp01_spec_detect_supply(
        charge_status.charging_active,
        charge_status.charge_done,
        charge_status.conflict,
        board_inputs.battery_mv);
    spec_inputs.key_pressed = board_inputs.key_pressed;
    spec_inputs.radar_active = board_inputs.radar_active;
    spec_inputs.radar_rising_edge = radar_event.rising_edge;
    spec_inputs.charging_active = charge_status.charging_active;
    spec_inputs.charge_done = charge_status.charge_done;
    spec_inputs.pump_sense_mv = board_inputs.pump_sense_mv;
    spec_inputs.battery_diag = power_diag;
    outputs = sp01_spec_product_tick(&g_spec_product, &spec_inputs);
#else
    app_inputs = app_inputs_from_board(&board_inputs, power_diag, pump_diag);
    outputs = sp01_app_tick(&g_app, &app_inputs);
#endif
#ifdef SP01_TOUCH_DIAG_TEST
    /* 诊断版的硬性安全兜底：不允许触摸输入启动水泵或雷达供电。 */
    outputs.pump_enable = APP_FALSE;
    outputs.radar_power_enable = APP_FALSE;
#endif
#ifdef SP01_RADAR_TEST
    outputs.pump_enable = APP_FALSE;
    outputs.radar_power_enable = APP_TRUE;
    outputs.led_mode = SP01_LED_OFF;
    outputs.buzzer_mode = SP01_BUZZER_OFF;
    if (g_radar_test_elapsed_ms < 5000UL) {
        g_radar_test_elapsed_ms += SP01_TICK_MS;
    } else if (radar_event.active == APP_FALSE) {
        if (g_radar_test_idle_ms < 2000UL) {
            g_radar_test_idle_ms += SP01_TICK_MS;
        }
        if (g_radar_test_idle_ms >= 2000UL) {
            g_radar_test_armed = APP_TRUE;
        }
    } else {
        g_radar_test_idle_ms = 0UL;
        if (g_radar_test_armed == APP_TRUE) {
            g_radar_test_armed = APP_FALSE;
            outputs.buzzer_mode = SP01_BUZZER_RADAR_TEST;
        }
    }
#endif
#ifdef SP01_PUMP_TEST
    /* 初始化和主循环使用同一输出；诊断仍采样，但不关断测试供电。 */
    outputs = sp01_pump_test_outputs();
#endif
#ifdef SP01_BATTERY_TEST
    /* 电池测试只观察 PB1/CHRG/DONE，水泵和雷达供电始终关闭。 */
    outputs.pump_enable = APP_FALSE;
    outputs.radar_power_enable = APP_FALSE;
    outputs.led_mode = sp01_battery_test_led_mode(power_diag, &charge_status);
    outputs.buzzer_mode = SP01_BUZZER_OFF;
#endif
#ifdef SP01_BATTERY_PUMP_TEST
    /* 电池台架只验证供电能力：PB2 持续高电平，不受计时影响。 */
    outputs.pump_enable = sp01_battery_pump_test_update(&g_battery_pump_test);
    outputs.radar_power_enable = APP_FALSE;
    outputs.led_mode = SP01_LED_OFF;
    outputs.buzzer_mode = SP01_BUZZER_OFF;
#endif
#ifdef SP01_PUMP_ADC_TEST
    /* PA3/ADC_XQ 诊断版只运行一轮受控采样，不走产品状态机。 */
    outputs = sp01_pump_adc_test_update(&g_pump_adc_test,
                                        board_inputs.key_pressed,
                                        pump_diag);
#endif
#ifdef SP01_PUMP_ADC_HOLD_TEST
    /* PA3台架版由触摸键手动切换水泵，ADC诊断结果不自动停泵。 */
    outputs = sp01_pump_adc_hold_test_update(&g_pump_adc_hold_test,
                                             board_inputs.key_pressed,
                                             g_sp01_debug_raw_inputs.key_pressed);
#endif
#if defined(SP01_RGB_TUNER_TEST) || defined(SP01_RGB_TEST) || defined(SP01_RGB_SINGLE_TEST) || \
    defined(SP01_RGB_ISOLATED_PA5_TEST) || defined(SP01_RGB_ISOLATED_PB3_TEST) || \
    defined(SP01_RGB_COMBO_PA5_PA1_TEST) || defined(SP01_RGB_COMBO_SCAN_TEST) || \
    defined(SP01_RGB_ALL_ON_TEST) || defined(SP01_RGB_WHITE_BALANCE_TEST)
    /* RGB 识别版覆盖所有业务输出，三个执行器保持关闭。 */
    outputs = sp01_rgb_test_tick(&g_rgb_test);
#endif
    outputs.buzzer_mode = sp01_buzzer_pattern_update(&g_buzzer_pattern,
                                                     outputs.buzzer_mode);
    outputs.buzzer_frequency_hz = sp01_buzzer_pattern_frequency_hz(&g_buzzer_pattern);
    outputs.buzzer_duty_percent = sp01_buzzer_pattern_duty_percent(&g_buzzer_pattern);
    applied_outputs = outputs;
#ifdef SP01_BENCH_NO_PUMP
    applied_outputs.pump_enable = APP_FALSE;
#endif
    board_apply_outputs(&applied_outputs);
    g_last_outputs = applied_outputs;

#ifndef SP01_RELEASE_BUILD
    g_sp01_debug_inputs = board_inputs;
    g_sp01_debug_power_snapshot = sp01_power_monitor_snapshot(&g_power_monitor);
    g_sp01_debug_outputs = outputs;
    g_sp01_debug_applied_outputs = applied_outputs;
    g_sp01_debug_pump_snapshot = sp01_pump_monitor_snapshot(&g_pump_monitor,
                                                            board_inputs.pump_sense_mv);
    g_sp01_debug_radar_event = radar_event;
    g_sp01_debug_charge_status = charge_status;
    g_sp01_debug_state = g_app.state;
    g_sp01_debug_fault = g_app.fault_code;
#ifdef SP01_FULL_TEST
    g_sp01_debug_full_power_state = g_full_test.power_state;
    g_sp01_debug_full_night_mode = g_full_test.night_mode_enabled;
    g_sp01_debug_full_radar_cooldown_ms = g_full_test.radar_cooldown_ms;
#endif
#ifdef SP01_SPEC_TEST
    g_sp01_debug_spec_state = g_spec_product.state;
    g_sp01_debug_spec_supply = g_spec_product.supply;
    g_sp01_debug_spec_night_mode = g_spec_product.night_mode;
#endif
#endif
}

int main(void)
{
    sp01_system_init();

    while (1) {
        if (board_take_10ms_tick() == APP_TRUE) {
            sp01_system_step();
            /* 只有完整跑完一轮采样、决策和输出后才喂狗。 */
#ifndef SP01_RELEASE_BUILD
            g_sp01_debug_uptime_ms += SP01_TICK_MS;
            if (g_sp01_debug_watchdog_refresh_enabled == APP_TRUE) {
                board_watchdog_refresh();
            }
#else
            board_watchdog_refresh();
#endif
        }
    }

}
