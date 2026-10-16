#include <assert.h>

#include "app/app_config.h"
#include "app/sp01_spec_product.h"

static sp01_spec_product_inputs_t default_inputs(void)
{
    sp01_spec_product_inputs_t inputs;

    inputs.supply = SP01_SPEC_SUPPLY_UNKNOWN;
    inputs.key_pressed = APP_FALSE;
    inputs.radar_active = APP_FALSE;
    inputs.radar_rising_edge = APP_FALSE;
    inputs.charging_active = APP_FALSE;
    inputs.charge_done = APP_FALSE;
    /* 主机测试默认模拟正常水位的 PA3 读数。 */
    inputs.pump_sense_mv = 900U;
    inputs.battery_diag = SP01_POWER_DIAG_NORMAL;
    return inputs;
}

static sp01_outputs_t tick(sp01_spec_product_t *product,
                           sp01_spec_product_inputs_t *inputs)
{
    return sp01_spec_product_tick(product, inputs);
}

static void wait_ticks(sp01_spec_product_t *product,
                       sp01_spec_product_inputs_t *inputs,
                       app_u32 count)
{
    app_u32 index;

    for (index = 0UL; index < count; index++) {
        (void)tick(product, inputs);
    }
}

static void short_press(sp01_spec_product_t *product,
                        sp01_spec_product_inputs_t *inputs)
{
    inputs->key_pressed = APP_TRUE;
    (void)tick(product, inputs);
    inputs->key_pressed = APP_FALSE;
    (void)tick(product, inputs);
}

static void long_press(sp01_spec_product_t *product,
                       sp01_spec_product_inputs_t *inputs)
{
    app_u32 index;

    inputs->key_pressed = APP_TRUE;
    for (index = 0UL; index < SP01_SPEC_POWER_PRESS_MS / SP01_TICK_MS; index++) {
        (void)tick(product, inputs);
    }
    inputs->key_pressed = APP_FALSE;
    (void)tick(product, inputs);
}

static void boot_to_battery_off(sp01_spec_product_t *product,
                                sp01_spec_product_inputs_t *inputs)
{
    inputs->supply = SP01_SPEC_SUPPLY_BATTERY;
    wait_ticks(product, inputs, SP01_SPEC_SUPPLY_DEBOUNCE_TICKS);
    assert(product->state == SP01_SPEC_STATE_OFF);
    assert(product->supply == SP01_SPEC_SUPPLY_BATTERY);
    assert(product->powered_on == APP_FALSE);
}

static void battery_power_on(sp01_spec_product_t *product,
                             sp01_spec_product_inputs_t *inputs)
{
    long_press(product, inputs);
    assert(product->powered_on == APP_TRUE);
    assert(product->state == SP01_SPEC_STATE_BATTERY_READY);
}

static void radar_wait_until_armed(sp01_spec_product_t *product,
                                   sp01_spec_product_inputs_t *inputs)
{
#if defined(SP01_SPEC_TEST004) || defined(SP01_SPEC_TEST005) || \
    defined(SP01_SPEC_TEST006) || defined(SP01_SPEC_TEST007)
    wait_ticks(product,
               inputs,
               (SP01_SPEC_RADAR_STARTUP_WARMUP_MS +
                SP01_SPEC_RADAR_IDLE_ARM_MS) / SP01_TICK_MS + 2UL);
    assert(product->radar_armed == APP_TRUE);
#else
    (void)tick(product, inputs);
    assert(product->radar_armed == APP_TRUE);
#endif
}

static void test_supply_detection_uses_chrg_or_done(void)
{
    assert(sp01_spec_detect_supply(APP_TRUE, APP_FALSE, APP_FALSE, 0U) ==
           SP01_SPEC_SUPPLY_TYPEC);
    assert(sp01_spec_detect_supply(APP_FALSE, APP_TRUE, APP_FALSE, 0U) ==
           SP01_SPEC_SUPPLY_TYPEC);
    /* Type-C 经 D3 抬高 BAT 电压，即使无电池、CHRG/DONE 都未有效也应识别为 Type-C。 */
    assert(sp01_spec_detect_supply(APP_FALSE, APP_FALSE, APP_FALSE, 4500U) ==
           SP01_SPEC_SUPPLY_TYPEC);
    /* 单节锂电池电压仍应识别为电池供电。 */
    assert(sp01_spec_detect_supply(APP_FALSE, APP_FALSE, APP_FALSE, 4200U) ==
           SP01_SPEC_SUPPLY_BATTERY);
    assert(sp01_spec_detect_supply(APP_FALSE, APP_FALSE, APP_FALSE, 3500U) ==
           SP01_SPEC_SUPPLY_BATTERY);
    assert(sp01_spec_detect_supply(APP_FALSE, APP_FALSE, APP_FALSE, 0U) ==
           SP01_SPEC_SUPPLY_UNKNOWN);
    assert(sp01_spec_detect_supply(APP_TRUE, APP_TRUE, APP_TRUE, 3500U) ==
           SP01_SPEC_SUPPLY_UNKNOWN);
}

static void test_boot_and_unknown_supply_are_safe(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;

    sp01_spec_product_init(&product);
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_BOOT);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.radar_power_enable == APP_FALSE);

    wait_ticks(&product, &inputs,
               SP01_SPEC_BOOT_SETTLE_MS / SP01_TICK_MS);
    assert(product.state == SP01_SPEC_STATE_OFF);
    outputs = tick(&product, &inputs);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.radar_power_enable == APP_FALSE);
}

static void test_typec_forces_continuous_and_reports_charge_state(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;
    app_u16 tick_index;

    inputs.supply = SP01_SPEC_SUPPLY_TYPEC;
    inputs.charging_active = APP_TRUE;
    sp01_spec_product_init(&product);
    wait_ticks(&product,
               &inputs,
               SP01_SPEC_SUPPLY_DEBOUNCE_TICKS - 1U);
    assert(product.state == SP01_SPEC_STATE_OFF);
    assert(product.supply == SP01_SPEC_SUPPLY_UNKNOWN);

    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS);
    assert(outputs.buzzer_mode == SP01_BUZZER_POWER_ON);

    outputs = tick(&product, &inputs);
    assert(outputs.buzzer_mode == SP01_BUZZER_OFF);
    assert(outputs.pump_enable == APP_TRUE);
    assert(outputs.led_mode == SP01_LED_PRODUCT_CHARGING);

    /* 少于 1 秒的充电状态丢失不能切模式、停泵或重复触发开机提示音。 */
    inputs.supply = SP01_SPEC_SUPPLY_UNKNOWN;
    inputs.charging_active = APP_FALSE;
    for (tick_index = 0U;
         tick_index < (SP01_SPEC_SUPPLY_DEBOUNCE_TICKS - 1U);
         tick_index++) {
        outputs = tick(&product, &inputs);
        assert(outputs.buzzer_mode == SP01_BUZZER_OFF);
        assert(outputs.pump_enable == APP_TRUE);
        assert(product.supply == SP01_SPEC_SUPPLY_TYPEC);
    }
    inputs.supply = SP01_SPEC_SUPPLY_TYPEC;
    inputs.charging_active = APP_TRUE;
    outputs = tick(&product, &inputs);
    assert(outputs.buzzer_mode == SP01_BUZZER_OFF);
    assert(product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS);

    inputs.charging_active = APP_FALSE;
    inputs.charge_done = APP_TRUE;
    outputs = tick(&product, &inputs);
    assert(outputs.pump_enable == APP_TRUE);
    assert(outputs.led_mode == SP01_LED_PRODUCT_FULL);

    inputs.charge_done = APP_FALSE;
    outputs = tick(&product, &inputs);
    assert(outputs.pump_enable == APP_TRUE);
    assert(outputs.led_mode == SP01_LED_PRODUCT_CONTINUOUS);

    /* 有线模式短按只切换指示灯，水泵仍保持持续出水。 */
    short_press(&product, &inputs);
    outputs = tick(&product, &inputs);
    assert(product.night_mode == APP_TRUE);
    assert(outputs.pump_enable == APP_TRUE);
    assert(outputs.led_mode == SP01_LED_OFF);

    long_press(&product, &inputs);
    assert(product.powered_on == APP_FALSE);
    assert(product.state == SP01_SPEC_STATE_OFF);
}

static void test_typec_unplug_falls_back_to_battery_without_power_cycle(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;

    inputs.supply = SP01_SPEC_SUPPLY_TYPEC;
    sp01_spec_product_init(&product);
    wait_ticks(&product, &inputs, SP01_SPEC_SUPPLY_DEBOUNCE_TICKS - 1U);
    outputs = tick(&product, &inputs);
    assert(product.supply == SP01_SPEC_SUPPLY_TYPEC);
    assert(product.powered_on == APP_TRUE);
    assert(product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS);
    assert(outputs.pump_enable == APP_TRUE);

    /* Type-C 拔出后仍有电池：供电识别去抖完成后应保持开机并进入电池待机。 */
    inputs.supply = SP01_SPEC_SUPPLY_BATTERY;
    wait_ticks(&product, &inputs, SP01_SPEC_SUPPLY_DEBOUNCE_TICKS);
    outputs = tick(&product, &inputs);
    assert(product.supply == SP01_SPEC_SUPPLY_BATTERY);
    assert(product.powered_on == APP_TRUE);
    assert(product.state == SP01_SPEC_STATE_BATTERY_READY);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.radar_power_enable == APP_TRUE);
    assert(outputs.buzzer_mode == SP01_BUZZER_OFF);
}

static void test_battery_radar_run_and_lockout(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;

    sp01_spec_product_init(&product);
    boot_to_battery_off(&product, &inputs);
    battery_power_on(&product, &inputs);

    /* 布防后才允许一个新的低到高变化触发出水。 */
    radar_wait_until_armed(&product, &inputs);
    inputs.radar_active = APP_TRUE;
    inputs.radar_rising_edge = APP_TRUE;
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_SENSOR_RUN);
    assert(outputs.pump_enable == APP_TRUE);
    assert(outputs.radar_power_enable == APP_TRUE);

    inputs.radar_rising_edge = APP_FALSE;
    wait_ticks(&product, &inputs,
               (SP01_SPEC_SENSOR_RUN_MS / SP01_TICK_MS) - 1UL);
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_SENSOR_LOCKOUT);
    assert(outputs.pump_enable == APP_FALSE);
    assert(product.lockout_remaining_ms == SP01_SPEC_SENSOR_LOCKOUT_MS);

    wait_ticks(&product, &inputs,
               SP01_SPEC_SENSOR_LOCKOUT_MS / SP01_TICK_MS);
    assert(product.state == SP01_SPEC_STATE_BATTERY_READY);
    assert(product.powered_on == APP_TRUE);
}

#if defined(SP01_SPEC_TEST004) || defined(SP01_SPEC_TEST005) || \
    defined(SP01_SPEC_TEST006) || defined(SP01_SPEC_TEST007)
static void test_radar_startup_and_cooldown_do_not_start_pump(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;

    sp01_spec_product_init(&product);
    boot_to_battery_off(&product, &inputs);
    battery_power_on(&product, &inputs);

    /* 模拟雷达上电时输出跳高：5 秒启动等待期间必须忽略。 */
    inputs.radar_active = APP_TRUE;
    inputs.radar_rising_edge = APP_TRUE;
    outputs = tick(&product, &inputs);
    assert(outputs.pump_enable == APP_FALSE);
    assert(product.state == SP01_SPEC_STATE_BATTERY_READY);
    inputs.radar_rising_edge = APP_FALSE;
    wait_ticks(&product, &inputs,
               SP01_SPEC_RADAR_STARTUP_WARMUP_MS / SP01_TICK_MS + 2UL);
    assert(product.radar_armed == APP_FALSE);
    assert(product.state == SP01_SPEC_STATE_BATTERY_READY);

    /* 活动信号消失后要持续无人 2 秒，布防本身不能启动水泵。 */
    inputs.radar_active = APP_FALSE;
    inputs.radar_rising_edge = APP_FALSE;
    wait_ticks(&product, &inputs,
               SP01_SPEC_RADAR_IDLE_ARM_MS / SP01_TICK_MS + 2UL);
    outputs = tick(&product, &inputs);
    assert(product.radar_armed == APP_TRUE);
    assert(outputs.pump_enable == APP_FALSE);

    /* 只有新的雷达上升沿才启动一次 15 秒出水。 */
    inputs.radar_active = APP_TRUE;
    inputs.radar_rising_edge = APP_TRUE;
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_SENSOR_RUN);
    assert(outputs.pump_enable == APP_TRUE);
    inputs.radar_rising_edge = APP_FALSE;
    wait_ticks(&product, &inputs,
               SP01_SPEC_SENSOR_RUN_MS / SP01_TICK_MS - 1UL);
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_SENSOR_LOCKOUT);
    assert(outputs.pump_enable == APP_FALSE);

    /* 冷却期间以及冷却结束后的无人等待都不能自行启动水泵。 */
    inputs.radar_active = APP_FALSE;
    wait_ticks(&product, &inputs,
               SP01_SPEC_SENSOR_LOCKOUT_MS / SP01_TICK_MS);
    assert(product.state == SP01_SPEC_STATE_BATTERY_READY);
    wait_ticks(&product, &inputs,
               SP01_SPEC_RADAR_IDLE_ARM_MS / SP01_TICK_MS + 2UL);
    outputs = tick(&product, &inputs);
    assert(product.radar_armed == APP_TRUE);
    assert(outputs.pump_enable == APP_FALSE);
    wait_ticks(&product, &inputs, 3000UL); /* 再观察 30 秒无人状态。 */
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_BATTERY_READY);
    assert(outputs.pump_enable == APP_FALSE);

    /* 3 分钟冷却之后的真实新动作仍需及时触发。 */
    inputs.radar_active = APP_TRUE;
    inputs.radar_rising_edge = APP_TRUE;
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_SENSOR_RUN);
    assert(outputs.pump_enable == APP_TRUE);
}
#endif

#if defined(SP01_SPEC_TEST005)
static void test_typec_low_water_requires_full_average_below_threshold(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;
    app_u32 index;

    inputs.supply = SP01_SPEC_SUPPLY_TYPEC;
    inputs.pump_sense_mv = 900U;
    sp01_spec_product_init(&product);
    wait_ticks(&product, &inputs, SP01_SPEC_SUPPLY_DEBOUNCE_TICKS - 1U);
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS);
    assert(outputs.pump_enable == APP_TRUE);

    /* 先越过 6 秒启动忽略期，并确认正常水位高读数不会误停泵。 */
    wait_ticks(&product,
               &inputs,
               (SP01_SPEC_LOW_WATER_STARTUP_IGNORE_MS +
                2UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS) /
                   SP01_TICK_MS);
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS);
    assert(outputs.pump_enable == APP_TRUE);

    /* 一个瞬间低值会被 1 秒平均过滤掉。 */
    inputs.pump_sense_mv = 500U;
    outputs = tick(&product, &inputs);
    assert(outputs.pump_enable == APP_TRUE);
    inputs.pump_sense_mv = 900U;
    wait_ticks(&product,
               &inputs,
               SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS / SP01_TICK_MS - 1UL);
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS);
    assert(outputs.pump_enable == APP_TRUE);

    /* 等于 650mV 不触发；严格低于 650mV 的完整 1 秒均值才触发。 */
    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_THRESHOLD_MV;
    wait_ticks(&product,
               &inputs,
               SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS / SP01_TICK_MS);
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS);
    assert(outputs.pump_enable == APP_TRUE);

    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_THRESHOLD_MV - 1U;
    for (index = 0UL;
         index < (SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS / SP01_TICK_MS) + 2UL;
         index++) {
        outputs = tick(&product, &inputs);
        if (product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT) {
            break;
        }
    }
    assert(product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT);
    assert(product.fault_code == SP01_FAULT_LOW_WATER);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.led_mode == SP01_LED_LOW_WATER);
    assert(outputs.buzzer_mode == SP01_BUZZER_LOW_WATER);

    /* 低水位红灯每秒闪烁：先亮 500ms，再灭 500ms。 */
    wait_ticks(&product,
               &inputs,
               SP01_SPEC_LOW_WATER_LED_ON_MS / SP01_TICK_MS - 1UL);
    outputs = tick(&product, &inputs);
    assert(outputs.led_mode == SP01_LED_OFF);
    wait_ticks(&product,
               &inputs,
               (SP01_SPEC_LOW_WATER_LED_PERIOD_MS -
                SP01_SPEC_LOW_WATER_LED_ON_MS) / SP01_TICK_MS - 1UL);
    outputs = tick(&product, &inputs);
    assert(outputs.led_mode == SP01_LED_LOW_WATER);

    /* 低水位锁定不会自动重试，持续插 Type-C 时水泵仍保持关闭。 */
    wait_ticks(&product, &inputs, 300UL);
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.led_mode == SP01_LED_LOW_WATER);

    /* 人工长按关机再开机后才清除锁定，并重新从启动保护期开始计时。 */
    long_press(&product, &inputs);
    assert(product.powered_on == APP_FALSE);
    assert(product.state == SP01_SPEC_STATE_OFF);
    inputs.pump_sense_mv = 900U;
    long_press(&product, &inputs);
    outputs = tick(&product, &inputs);
    assert(product.powered_on == APP_TRUE);
    assert(product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS);
    assert(product.fault_code == SP01_FAULT_NONE);
    assert(outputs.pump_enable == APP_TRUE);
}

static void test_battery_radar_pump_also_uses_low_water_protection(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;

    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_THRESHOLD_MV - 100U;
    sp01_spec_product_init(&product);
    boot_to_battery_off(&product, &inputs);
    battery_power_on(&product, &inputs);
    radar_wait_until_armed(&product, &inputs);
    inputs.radar_active = APP_TRUE;
    inputs.radar_rising_edge = APP_TRUE;
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_SENSOR_RUN);
    assert(outputs.pump_enable == APP_TRUE);

    inputs.radar_rising_edge = APP_FALSE;
    wait_ticks(&product,
               &inputs,
               (SP01_SPEC_LOW_WATER_STARTUP_IGNORE_MS +
                SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS) /
                   SP01_TICK_MS - 1UL);
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.radar_power_enable == APP_FALSE);
}
#endif

#if defined(SP01_SPEC_TEST006)
static void test_typec_low_water_needs_consecutive_low_windows(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;
    app_u32 index;

    inputs.supply = SP01_SPEC_SUPPLY_TYPEC;
    inputs.pump_sense_mv = 900U;
    sp01_spec_product_init(&product);
    wait_ticks(&product, &inputs, SP01_SPEC_SUPPLY_DEBOUNCE_TICKS - 1U);
    outputs = tick(&product, &inputs);
    assert(outputs.pump_enable == APP_TRUE);
    wait_ticks(&product,
               &inputs,
               (SP01_SPEC_LOW_WATER_STARTUP_IGNORE_MS +
                SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS) / SP01_TICK_MS);

    /* 两个低窗口后恢复正常，连续计数必须清零，不能误报。 */
    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_THRESHOLD_MV - 50U;
    wait_ticks(&product,
               &inputs,
               2UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS / SP01_TICK_MS);
    inputs.pump_sense_mv = 900U;
    wait_ticks(&product,
               &inputs,
               SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS / SP01_TICK_MS);
    assert(product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS);

    /* 随后出现三个连续低窗口，才允许进入低水位保护。 */
    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_THRESHOLD_MV - 50U;
    for (index = 0UL;
         index < (4UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS) /
                     SP01_TICK_MS;
         index++) {
        outputs = tick(&product, &inputs);
        if (product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT) {
            break;
        }
    }
    assert(product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT);
    assert(outputs.pump_enable == APP_FALSE);
}

static void test_typec_low_water_auto_recovers_after_refill(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;
    app_u32 index;
    app_bool_t retry_started = APP_FALSE;

    inputs.supply = SP01_SPEC_SUPPLY_TYPEC;
    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_THRESHOLD_MV - 50U;
    sp01_spec_product_init(&product);
    wait_ticks(&product, &inputs, SP01_SPEC_SUPPLY_DEBOUNCE_TICKS - 1U);
    outputs = tick(&product, &inputs);
    assert(outputs.pump_enable == APP_TRUE);

    /* 006 必须连续三个完整低值窗口才进入低水位锁定。 */
    for (index = 0UL;
         index < (SP01_SPEC_LOW_WATER_STARTUP_IGNORE_MS +
                  4UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS) /
                     SP01_TICK_MS;
         index++) {
        outputs = tick(&product, &inputs);
    }
    assert(product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT);
    assert(outputs.pump_enable == APP_FALSE);

    /* 首次低水位重试在锁定 30 秒时开始；试泵期间应重新打开水泵输出。 */
    inputs.pump_sense_mv = 900U;
    for (index = 0UL;
         index < SP01_SPEC_LOW_WATER_RETRY_1_AT_MS / SP01_TICK_MS + 100UL;
         index++) {
        outputs = tick(&product, &inputs);
        if (outputs.pump_enable == APP_TRUE) {
            retry_started = APP_TRUE;
            break;
        }
    }
    assert(retry_started == APP_TRUE);
    assert(product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT);

    /* 前两个有效窗口仍偏低，后两个窗口恢复正常；5 秒试泵应能在本次确认补水。 */
    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_RECOVERY_THRESHOLD_MV - 100U;
    wait_ticks(&product,
               &inputs,
               (3UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS -
                SP01_TICK_MS) / SP01_TICK_MS);
    assert(product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT);
    assert(product.low_water_retry_active == APP_TRUE);

    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_RECOVERY_THRESHOLD_MV + 150U;
    for (index = 0UL;
         index < SP01_SPEC_LOW_WATER_RETRY_RUN_MS / SP01_TICK_MS;
         index++) {
        outputs = tick(&product, &inputs);
        if (product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS) {
            break;
        }
    }
    assert(product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS);
    assert(product.fault_code == SP01_FAULT_NONE);
    assert(outputs.pump_enable == APP_TRUE);
}

static void test_battery_low_water_auto_recovers_and_low_battery_blocks_retry(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;
    app_u32 index;
    app_bool_t retry_started = APP_FALSE;

    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_THRESHOLD_MV - 50U;
    sp01_spec_product_init(&product);
    boot_to_battery_off(&product, &inputs);
    battery_power_on(&product, &inputs);
    radar_wait_until_armed(&product, &inputs);
    inputs.radar_active = APP_TRUE;
    inputs.radar_rising_edge = APP_TRUE;
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_SENSOR_RUN);
    assert(outputs.pump_enable == APP_TRUE);

    inputs.radar_rising_edge = APP_FALSE;
    for (index = 0UL;
         index < (SP01_SPEC_LOW_WATER_STARTUP_IGNORE_MS +
                  4UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS) /
                     SP01_TICK_MS;
         index++) {
        outputs = tick(&product, &inputs);
        if (product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT) {
            break;
        }
    }
    assert(product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT);
    assert(outputs.pump_enable == APP_FALSE);

    /* 电池低压时不允许自动试泵，即使首次重试时间已到也继续锁定。 */
    inputs.pump_sense_mv = 900U;
    inputs.battery_diag = SP01_POWER_DIAG_LOW;
    wait_ticks(&product,
               &inputs,
               SP01_SPEC_LOW_WATER_RETRY_1_AT_MS / SP01_TICK_MS + 10UL);
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT);
    assert(outputs.pump_enable == APP_FALSE);
    assert(product.low_water_retry_schedule_index == 1U);
    assert(product.low_water_retry_count == 0U);

    /* 第二个固定时间点仍处于低压，继续跳过，不把漏掉的试泵补到当前时刻。 */
    wait_ticks(&product,
               &inputs,
               (SP01_SPEC_LOW_WATER_RETRY_2_AT_MS -
                SP01_SPEC_LOW_WATER_RETRY_1_AT_MS) / SP01_TICK_MS + 10UL);
    outputs = tick(&product, &inputs);
    assert(product.low_water_retry_schedule_index == 2U);
    assert(product.low_water_retry_count == 0U);
    assert(outputs.pump_enable == APP_FALSE);

    /* 电池恢复正常后等到下一个固定时间点 5 分钟再试泵。 */
    inputs.battery_diag = SP01_POWER_DIAG_NORMAL;
    for (index = 0UL;
         index < (SP01_SPEC_LOW_WATER_RETRY_3_AT_MS -
                  SP01_SPEC_LOW_WATER_RETRY_2_AT_MS) / SP01_TICK_MS + 10UL;
         index++) {
        outputs = tick(&product, &inputs);
        if (outputs.pump_enable == APP_TRUE) {
            retry_started = APP_TRUE;
            break;
        }
    }
    assert(retry_started == APP_TRUE);
    assert(product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT);
    assert(product.low_water_retry_schedule_index == 3U);
    assert(product.low_water_retry_count == 1U);

    for (index = 0UL;
         index < (3UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS) /
                     SP01_TICK_MS;
         index++) {
        outputs = tick(&product, &inputs);
        if (product.state == SP01_SPEC_STATE_BATTERY_READY) {
            break;
        }
    }
    assert(product.state == SP01_SPEC_STATE_BATTERY_READY);
    assert(product.fault_code == SP01_FAULT_NONE);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.radar_power_enable == APP_TRUE);
}

static void test_low_water_retry_backoff_and_shutdown_after_30_minutes(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;
    app_u32 index;
    app_u8 retry_index;
    static const app_u32 retry_due_ms[SP01_SPEC_LOW_WATER_RETRY_COUNT] = {
        SP01_SPEC_LOW_WATER_RETRY_1_AT_MS,
        SP01_SPEC_LOW_WATER_RETRY_2_AT_MS,
        SP01_SPEC_LOW_WATER_RETRY_3_AT_MS,
        SP01_SPEC_LOW_WATER_RETRY_4_AT_MS,
        SP01_SPEC_LOW_WATER_RETRY_5_AT_MS,
        SP01_SPEC_LOW_WATER_RETRY_6_AT_MS
    };

    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_THRESHOLD_MV - 50U;
    sp01_spec_product_init(&product);
    boot_to_battery_off(&product, &inputs);
    battery_power_on(&product, &inputs);
    radar_wait_until_armed(&product, &inputs);
    inputs.radar_active = APP_TRUE;
    inputs.radar_rising_edge = APP_TRUE;
    outputs = tick(&product, &inputs);
    assert(outputs.pump_enable == APP_TRUE);

    inputs.radar_rising_edge = APP_FALSE;
    for (index = 0UL;
         index < (SP01_SPEC_LOW_WATER_STARTUP_IGNORE_MS +
                  4UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS) /
                     SP01_TICK_MS;
         index++) {
        outputs = tick(&product, &inputs);
        if (product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT) {
            break;
        }
    }
    assert(product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT);
    assert(outputs.pump_enable == APP_FALSE);

    /* 六次都从进入锁定的时刻算绝对时间，每次低水位试泵只运行约 3 秒。 */
    for (retry_index = 0U;
         retry_index < SP01_SPEC_LOW_WATER_RETRY_COUNT;
         retry_index++) {
        app_u32 ticks_until_due;

        ticks_until_due = (retry_due_ms[retry_index] -
                           product.low_water_lockout_elapsed_ms) /
                          SP01_TICK_MS + 2UL;
        for (index = 0UL;
             index < ticks_until_due &&
             product.low_water_retry_active == APP_FALSE;
             index++) {
            outputs = tick(&product, &inputs);
        }

        assert(product.low_water_retry_schedule_index == retry_index + 1U);
        assert(product.low_water_retry_count == retry_index + 1U);
        assert(product.low_water_retry_active == APP_TRUE);
        assert(product.low_water_lockout_elapsed_ms >=
               retry_due_ms[retry_index]);
        if (retry_index + 1U < SP01_SPEC_LOW_WATER_RETRY_COUNT) {
            assert(product.low_water_lockout_elapsed_ms <
                   retry_due_ms[retry_index + 1U]);
        }

        wait_ticks(&product,
                   &inputs,
                   SP01_SPEC_LOW_WATER_RETRY_RUN_MS / SP01_TICK_MS - 1UL);
        outputs = tick(&product, &inputs);
        assert(product.low_water_retry_active == APP_FALSE);

        if (retry_index + 1U < SP01_SPEC_LOW_WATER_RETRY_COUNT) {
            assert(product.powered_on == APP_TRUE);
            assert(outputs.pump_enable == APP_FALSE);
        }
    }

    /* 第六次试泵结束仍未恢复，才执行自动软件关机。 */
    assert(product.powered_on == APP_FALSE);
    assert(product.state == SP01_SPEC_STATE_OFF);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.radar_power_enable == APP_FALSE);
    assert(outputs.led_mode == SP01_LED_OFF);
}
#endif

#if defined(SP01_SPEC_TEST007)
static void test_spec_test007_retries_every_30_seconds_without_auto_shutdown(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;
    app_u32 index;
    app_u32 due_ms;
    app_u8 retry_index;

    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_THRESHOLD_MV - 50U;
    sp01_spec_product_init(&product);
    boot_to_battery_off(&product, &inputs);
    battery_power_on(&product, &inputs);
    radar_wait_until_armed(&product, &inputs);
    inputs.radar_active = APP_TRUE;
    inputs.radar_rising_edge = APP_TRUE;
    outputs = tick(&product, &inputs);
    assert(outputs.pump_enable == APP_TRUE);

    inputs.radar_rising_edge = APP_FALSE;
    for (index = 0UL;
         index < (SP01_SPEC_LOW_WATER_STARTUP_IGNORE_MS +
                  4UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS) /
                     SP01_TICK_MS;
         index++) {
        outputs = tick(&product, &inputs);
        if (product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT) {
            break;
        }
    }
    assert(product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT);
    assert(outputs.pump_enable == APP_FALSE);

    /* 007 每 30 秒复测一次；即使跨过 30 分钟仍保持低水位锁定并继续重试。 */
    for (retry_index = 0U;
         retry_index < 61U;
         retry_index++) {
        /* 每次试泵开始时计时器会清零，因此每一轮都只等待30秒。 */
        due_ms = SP01_SPEC_LOW_WATER_RETRY_INTERVAL_MS;
        while (product.low_water_lockout_elapsed_ms < due_ms &&
               product.low_water_retry_active == APP_FALSE) {
            outputs = tick(&product, &inputs);
            assert(product.powered_on == APP_TRUE);
            assert(outputs.pump_enable == product.low_water_retry_active);
        }

        /* 触发本轮后计时器会被清零，不能再用 elapsed_ms 与30秒比较。 */
        assert(product.low_water_retry_schedule_index == retry_index + 1U);
        assert(product.low_water_retry_count == retry_index + 1U);
        assert(product.low_water_retry_active == APP_TRUE);
        outputs = tick(&product, &inputs);
        assert(outputs.pump_enable == APP_TRUE);
        wait_ticks(&product,
                   &inputs,
                   SP01_SPEC_LOW_WATER_RETRY_RUN_MS / SP01_TICK_MS - 2UL);
        outputs = tick(&product, &inputs);

        assert(product.powered_on == APP_TRUE);
        assert(product.low_water_retry_active == APP_FALSE);
        assert(outputs.pump_enable == APP_FALSE);
    }

    /* 61 次复测已跨过原先的 30 分钟上限，仍保持低水位锁定。 */
    assert(product.powered_on == APP_TRUE);
    assert(product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.radar_power_enable == APP_FALSE);
}

static void test_spec_test007_typec_warning_and_protection(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;
    app_u32 index;

    /* 590mV 是新测正常水位下沿，不应误报预警。 */
    inputs.supply = SP01_SPEC_SUPPLY_TYPEC;
    inputs.pump_sense_mv = 590U;
    sp01_spec_product_init(&product);
    wait_ticks(&product, &inputs, SP01_SPEC_SUPPLY_DEBOUNCE_TICKS - 1U);
    outputs = tick(&product, &inputs);
    assert(outputs.pump_enable == APP_TRUE);

    wait_ticks(&product,
               &inputs,
               (SP01_SPEC_LOW_WATER_STARTUP_IGNORE_MS +
                2UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS) /
                   SP01_TICK_MS - 1UL);
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS);
    assert(product.low_water_warning_active == APP_FALSE);
    assert(outputs.pump_enable == APP_TRUE);

    /* 585mV 位于预警边界：连续两个均值确认后才开始白灯单闪。 */
    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_WARNING_THRESHOLD_MV;
    wait_ticks(&product,
               &inputs,
               2UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS / SP01_TICK_MS + 2UL);
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS);
    assert(product.low_water_warning_active == APP_TRUE);
    assert(outputs.pump_enable == APP_TRUE);
    assert(outputs.led_mode == SP01_LED_PRODUCT_LOW_WATER_WARNING_TYPEC);

    /* 白灯每秒只亮约0.1秒，其余时间熄灭，并持续重复。 */
    wait_ticks(&product,
               &inputs,
               SP01_SPEC_LOW_WATER_WARNING_ON_MS / SP01_TICK_MS);
    outputs = tick(&product, &inputs);
    assert(outputs.led_mode == SP01_LED_OFF);
    for (index = 0UL;
         index < (SP01_SPEC_LOW_WATER_WARNING_PERIOD_MS -
                  SP01_SPEC_LOW_WATER_WARNING_ON_MS) / SP01_TICK_MS - 1UL;
         index++) {
        outputs = tick(&product, &inputs);
    }
    outputs = tick(&product, &inputs);
    assert(outputs.led_mode == SP01_LED_PRODUCT_LOW_WATER_WARNING_TYPEC);
    assert(product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS);
    assert(outputs.pump_enable == APP_TRUE);

    /* 恢复门槛为625mV，必须严格高于625mV才清除预警。 */
    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_WARNING_RECOVERY_MV;
    wait_ticks(&product,
               &inputs,
               2UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS / SP01_TICK_MS);
    assert(product.low_water_warning_active == APP_TRUE);

    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_WARNING_RECOVERY_MV + 1U;
    for (index = 0UL;
         index < 3UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS / SP01_TICK_MS;
         index++) {
        outputs = tick(&product, &inputs);
        if (product.low_water_warning_active == APP_FALSE) {
            break;
        }
    }
    assert(product.low_water_warning_active == APP_FALSE);
    assert(outputs.led_mode == SP01_LED_PRODUCT_CONTINUOUS);
    assert(outputs.pump_enable == APP_TRUE);

    /* 450mV 是实测过低水位区间上沿，但高于420mV停泵线，不应停泵。 */
    inputs.pump_sense_mv = 450U;
    wait_ticks(&product,
               &inputs,
               3UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS / SP01_TICK_MS + 2UL);
    assert(product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS);
    assert(outputs.pump_enable == APP_TRUE);

    /* 420mV位于停泵边界：连续三个1秒窗口后才锁定并停泵。 */
    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_THRESHOLD_MV;
    for (index = 0UL; index < 1000UL; index++) {
        outputs = tick(&product, &inputs);
        if (product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT) {
            break;
        }
    }
    assert(product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT);
    assert(product.fault_code == SP01_FAULT_LOW_WATER);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.led_mode == SP01_LED_LOW_WATER);

    /* 恰好625mV不恢复；必须高于625mV，连续两个窗口才解除锁定。 */
    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_RECOVERY_THRESHOLD_MV;
    for (index = 0UL;
         index < SP01_SPEC_LOW_WATER_RETRY_1_AT_MS / SP01_TICK_MS +
                 SP01_SPEC_LOW_WATER_RETRY_RUN_MS / SP01_TICK_MS + 20UL;
         index++) {
        outputs = tick(&product, &inputs);
    }
    assert(product.state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT);

    inputs.pump_sense_mv = SP01_SPEC_LOW_WATER_RECOVERY_THRESHOLD_MV + 1U;
    for (index = 0UL;
         index < SP01_SPEC_LOW_WATER_RETRY_INTERVAL_MS / SP01_TICK_MS +
                 SP01_SPEC_LOW_WATER_RETRY_RUN_MS / SP01_TICK_MS + 20UL;
         index++) {
        outputs = tick(&product, &inputs);
        if (product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS) {
            break;
        }
    }
    assert(product.state == SP01_SPEC_STATE_TYPEC_CONTINUOUS);
    assert(product.fault_code == SP01_FAULT_NONE);
    assert(product.low_water_warning_active == APP_FALSE);
    assert(outputs.pump_enable == APP_TRUE);
}

static void test_spec_test007_battery_warning_persists_until_next_normal_radar_sample(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;
    app_u32 index;

    /* 第一次雷达出水测到550mV：电池灯持续绿闪，不触发停泵。 */
    inputs.pump_sense_mv = 550U;
    sp01_spec_product_init(&product);
    boot_to_battery_off(&product, &inputs);
    battery_power_on(&product, &inputs);
    radar_wait_until_armed(&product, &inputs);
    inputs.radar_active = APP_TRUE;
    inputs.radar_rising_edge = APP_TRUE;
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_SENSOR_RUN);
    assert(outputs.pump_enable == APP_TRUE);

    inputs.radar_rising_edge = APP_FALSE;
    wait_ticks(&product,
               &inputs,
               (SP01_SPEC_LOW_WATER_STARTUP_IGNORE_MS +
                2UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS) /
                   SP01_TICK_MS - 1UL);
    outputs = tick(&product, &inputs);
    assert(product.low_water_warning_active == APP_TRUE);
    assert(product.state == SP01_SPEC_STATE_SENSOR_RUN);
    assert(outputs.pump_enable == APP_TRUE);
    assert(outputs.led_mode == SP01_LED_PRODUCT_LOW_WATER_WARNING_BATTERY);

    /* 15秒出水正常结束；预警在电池冷却期间保留并继续提示。 */
    wait_ticks(&product,
               &inputs,
               (SP01_SPEC_SENSOR_RUN_MS -
                SP01_SPEC_LOW_WATER_STARTUP_IGNORE_MS -
                2UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS) /
                   SP01_TICK_MS);
    assert(product.state == SP01_SPEC_STATE_SENSOR_LOCKOUT);
    outputs = tick(&product, &inputs);
    assert(outputs.pump_enable == APP_FALSE);
    assert(product.low_water_warning_active == APP_TRUE);

    /* 冷却结束后等雷达重新布防，再以正常水位触发下一轮。 */
    inputs.radar_active = APP_FALSE;
    wait_ticks(&product,
               &inputs,
               SP01_SPEC_SENSOR_LOCKOUT_MS / SP01_TICK_MS +
               SP01_SPEC_RADAR_STARTUP_WARMUP_MS / SP01_TICK_MS +
               SP01_SPEC_RADAR_IDLE_ARM_MS / SP01_TICK_MS + 5UL);
    assert(product.state == SP01_SPEC_STATE_BATTERY_READY);
    assert(product.radar_armed == APP_TRUE);

    inputs.pump_sense_mv = 900U;
    inputs.radar_active = APP_TRUE;
    inputs.radar_rising_edge = APP_TRUE;
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_SENSOR_RUN);
    assert(outputs.pump_enable == APP_TRUE);
    inputs.radar_rising_edge = APP_FALSE;

    for (index = 0UL;
         index < (SP01_SPEC_LOW_WATER_STARTUP_IGNORE_MS +
                  SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS) / SP01_TICK_MS + 20UL;
         index++) {
        outputs = tick(&product, &inputs);
        if (product.low_water_warning_active == APP_FALSE) {
            break;
        }
    }
    assert(product.low_water_warning_active == APP_FALSE);
    assert(outputs.led_mode == SP01_LED_SENSOR_READY);
    assert(outputs.pump_enable == APP_TRUE);
}

static void test_spec_test007_low_battery_and_water_warning_uses_red_single_flash(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;
    app_u32 index;

    /* 只有低电量时红灯常亮；水位尚未进入预警时不应闪烁。 */
    inputs.pump_sense_mv = 550U;
    inputs.battery_diag = SP01_POWER_DIAG_LOW;
    sp01_spec_product_init(&product);
    boot_to_battery_off(&product, &inputs);
    battery_power_on(&product, &inputs);
    outputs = tick(&product, &inputs);
    assert(product.low_water_warning_active == APP_FALSE);
    assert(outputs.led_mode == SP01_LED_PRODUCT_LOW_BATTERY);
    outputs = tick(&product, &inputs);
    assert(outputs.led_mode == SP01_LED_PRODUCT_LOW_BATTERY);

    /* 电池低压叠加水位过低预警时，改为红灯每秒单闪约0.1秒。 */
    radar_wait_until_armed(&product, &inputs);
    inputs.radar_active = APP_TRUE;
    inputs.radar_rising_edge = APP_TRUE;
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_SENSOR_RUN);
    assert(outputs.pump_enable == APP_TRUE);
    inputs.radar_rising_edge = APP_FALSE;

    for (index = 0UL;
         index < (SP01_SPEC_LOW_WATER_STARTUP_IGNORE_MS +
                  2UL * SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS) /
                     SP01_TICK_MS + 5UL;
         index++) {
        outputs = tick(&product, &inputs);
    }
    assert(product.low_water_warning_active == APP_TRUE);
    assert(product.state == SP01_SPEC_STATE_SENSOR_RUN);
    assert(outputs.pump_enable == APP_TRUE);

    /* 从闪烁周期起点验证亮约100ms、灭约900ms，然后重复。 */
    product.low_water_led_elapsed_ms = 0UL;
    outputs = tick(&product, &inputs);
    assert(outputs.led_mode == SP01_LED_PRODUCT_LOW_BATTERY);
    for (index = 1UL;
         index < SP01_SPEC_LOW_WATER_WARNING_ON_MS / SP01_TICK_MS;
         index++) {
        outputs = tick(&product, &inputs);
    }
    assert(outputs.led_mode == SP01_LED_PRODUCT_LOW_BATTERY);
    outputs = tick(&product, &inputs);
    assert(outputs.led_mode == SP01_LED_OFF);
    wait_ticks(&product,
               &inputs,
               (SP01_SPEC_LOW_WATER_WARNING_PERIOD_MS -
                SP01_SPEC_LOW_WATER_WARNING_ON_MS) / SP01_TICK_MS - 1UL);
    assert(outputs.led_mode == SP01_LED_OFF);
    outputs = tick(&product, &inputs);
    assert(outputs.led_mode == SP01_LED_PRODUCT_LOW_BATTERY);

    /* 电池恢复正常后，同一个水位预警仍按原逻辑显示绿灯单闪。 */
    inputs.battery_diag = SP01_POWER_DIAG_NORMAL;
    outputs = tick(&product, &inputs);
    assert(product.low_water_warning_active == APP_TRUE);
    assert(outputs.led_mode == SP01_LED_PRODUCT_LOW_WATER_WARNING_BATTERY);
}
#endif

static void test_battery_short_press_toggles_night_mode(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;

    sp01_spec_product_init(&product);
    boot_to_battery_off(&product, &inputs);
    battery_power_on(&product, &inputs);

    short_press(&product, &inputs);
    outputs = tick(&product, &inputs);
    assert(product.night_mode == APP_TRUE);
    assert(outputs.pump_enable == APP_FALSE);
    assert(outputs.led_mode == SP01_LED_OFF);

    short_press(&product, &inputs);
    outputs = tick(&product, &inputs);
    assert(product.night_mode == APP_FALSE);
    assert(outputs.led_mode == SP01_LED_SENSOR_READY);
}

static void test_three_second_power_toggle(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();

    sp01_spec_product_init(&product);
    boot_to_battery_off(&product, &inputs);
    battery_power_on(&product, &inputs);
    long_press(&product, &inputs);
    assert(product.powered_on == APP_FALSE);
    assert(product.state == SP01_SPEC_STATE_OFF);

    long_press(&product, &inputs);
    assert(product.powered_on == APP_TRUE);
    assert(product.state == SP01_SPEC_STATE_BATTERY_READY);
}

static void test_power_toggle_occurs_while_key_is_still_held(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;
    app_u32 index;

    sp01_spec_product_init(&product);
    boot_to_battery_off(&product, &inputs);

    /* 逻辑层收到按下后，累计到 3 秒前仍应处于关机。 */
    inputs.key_pressed = APP_TRUE;
    for (index = 0UL;
         index < (SP01_SPEC_POWER_PRESS_MS / SP01_TICK_MS) - 1UL;
         index++) {
        outputs = tick(&product, &inputs);
        assert(product.powered_on == APP_FALSE);
        assert(outputs.pump_enable == APP_FALSE);
    }

    /* 满 3 秒的这个 tick 就开机，用户无需松手；电池模式不会自动开泵。 */
    outputs = tick(&product, &inputs);
    assert(product.powered_on == APP_TRUE);
    assert(product.state == SP01_SPEC_STATE_BATTERY_READY);
    assert(outputs.buzzer_mode == SP01_BUZZER_POWER_ON);
    assert(outputs.pump_enable == APP_FALSE);

    /* 持续按住不会重复开关机。 */
    for (index = 0UL; index < 100UL; index++) {
        outputs = tick(&product, &inputs);
        assert(product.powered_on == APP_TRUE);
        assert(outputs.buzzer_mode == SP01_BUZZER_OFF);
    }

    /* 松手只复位手势计时，不得再触发夜间模式或第二次开关机。 */
    inputs.key_pressed = APP_FALSE;
    outputs = tick(&product, &inputs);
    assert(product.powered_on == APP_TRUE);
    assert(product.night_mode == APP_FALSE);
    assert(outputs.buzzer_mode == SP01_BUZZER_OFF);

    /* 再按住 3 秒，关机反馈同样发生在松手前。 */
    inputs.key_pressed = APP_TRUE;
    for (index = 0UL; index < SP01_SPEC_POWER_PRESS_MS / SP01_TICK_MS; index++) {
        outputs = tick(&product, &inputs);
    }
    assert(product.powered_on == APP_FALSE);
    assert(product.state == SP01_SPEC_STATE_OFF);
    assert(outputs.buzzer_mode == SP01_BUZZER_POWER_OFF);

    inputs.key_pressed = APP_FALSE;
    outputs = tick(&product, &inputs);
    assert(product.powered_on == APP_FALSE);
    assert(outputs.buzzer_mode == SP01_BUZZER_OFF);
}

static void test_three_hour_fur_flush(void)
{
    sp01_spec_product_t product;
    sp01_spec_product_inputs_t inputs = default_inputs();
    sp01_outputs_t outputs;

    sp01_spec_product_init(&product);
    boot_to_battery_off(&product, &inputs);
    battery_power_on(&product, &inputs);
    wait_ticks(&product, &inputs,
               SP01_SPEC_FUR_FLUSH_INTERVAL_MS / SP01_TICK_MS);
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_FUR_FLUSH);
    assert(outputs.pump_enable == APP_TRUE);

    wait_ticks(&product, &inputs,
               (SP01_SPEC_FUR_FLUSH_RUN_MS / SP01_TICK_MS) - 1UL);
    outputs = tick(&product, &inputs);
    assert(product.state == SP01_SPEC_STATE_BATTERY_READY);
    assert(outputs.pump_enable == APP_FALSE);
    assert(product.fur_flush_due == APP_FALSE);
}

int main(void)
{
    test_supply_detection_uses_chrg_or_done();
    test_boot_and_unknown_supply_are_safe();
    test_typec_forces_continuous_and_reports_charge_state();
    test_typec_unplug_falls_back_to_battery_without_power_cycle();
    test_battery_radar_run_and_lockout();
#if defined(SP01_SPEC_TEST004) || defined(SP01_SPEC_TEST005) || \
    defined(SP01_SPEC_TEST006) || defined(SP01_SPEC_TEST007)
    test_radar_startup_and_cooldown_do_not_start_pump();
#endif
#if defined(SP01_SPEC_TEST005)
    test_typec_low_water_requires_full_average_below_threshold();
    test_battery_radar_pump_also_uses_low_water_protection();
#endif
#if defined(SP01_SPEC_TEST006)
    test_typec_low_water_needs_consecutive_low_windows();
    test_typec_low_water_auto_recovers_after_refill();
    test_battery_low_water_auto_recovers_and_low_battery_blocks_retry();
    test_low_water_retry_backoff_and_shutdown_after_30_minutes();
#endif
#if defined(SP01_SPEC_TEST007)
    test_spec_test007_retries_every_30_seconds_without_auto_shutdown();
    test_spec_test007_typec_warning_and_protection();
    test_spec_test007_battery_warning_persists_until_next_normal_radar_sample();
    test_spec_test007_low_battery_and_water_warning_uses_red_single_flash();
#endif
    test_battery_short_press_toggles_night_mode();
    test_three_second_power_toggle();
    test_power_toggle_occurs_while_key_is_still_held();
    test_three_hour_fur_flush();
    return 0;
}
