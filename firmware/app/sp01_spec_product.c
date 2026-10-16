#include "sp01_spec_product.h"
#include "app_config.h"

static app_u32 sp01_spec_accumulate(app_u32 value,
                                    app_u32 tick_ms,
                                    app_u32 limit_ms)
{
    if (value >= limit_ms || tick_ms >= (limit_ms - value)) {
        return limit_ms;
    }
    return value + tick_ms;
}

static void sp01_spec_set_state(sp01_spec_product_t *product,
                                sp01_spec_state_t state)
{
    product->state = state;
    product->state_elapsed_ms = 0UL;
    product->low_water_led_elapsed_ms = 0UL;
}

static void sp01_spec_pump_adc_reset(sp01_spec_product_t *product)
{
    product->pump_run_elapsed_ms = 0UL;
    product->pump_adc_sum_mv = 0UL;
    product->pump_adc_sample_count = 0U;
    product->pump_adc_last_average_mv = 0U;
    product->low_water_consecutive_count = 0U;
    product->low_water_warning_consecutive_count = 0U;
    product->low_water_protection_consecutive_count = 0U;
    product->low_water_warning_active = APP_FALSE;
}

#if defined(SP01_SPEC_TEST006) || defined(SP01_SPEC_TEST007)
static void sp01_spec_power_off(sp01_spec_product_t *product);
#endif

#if defined(SP01_SPEC_TEST005) || defined(SP01_SPEC_TEST006) || \
    defined(SP01_SPEC_TEST007)
/*
 * 低水位验证版：水泵每次启动先忽略 6 秒，再计算连续 1 秒 PA3 均值。
 * 这里判断的是 ADC_XQ 毫伏，不把粗略换算的毫安当作精确测量值。
 */
static app_bool_t sp01_spec_low_water_update(sp01_spec_product_t *product,
                                             app_bool_t pump_requested,
                                             app_u16 pump_sense_mv)
{
    app_u32 window_samples;
    app_u16 average_mv;

    if (pump_requested != APP_TRUE) {
        /* 电池模式停泵时保留预警状态，等下一次雷达出水重新确认。 */
        product->pump_run_elapsed_ms = 0UL;
        product->pump_adc_sum_mv = 0UL;
        product->pump_adc_sample_count = 0U;
        product->pump_adc_last_average_mv = 0U;
        product->low_water_consecutive_count = 0U;
        product->low_water_warning_consecutive_count = 0U;
        product->low_water_protection_consecutive_count = 0U;
        return APP_FALSE;
    }

    if (product->pump_run_elapsed_ms < SP01_SPEC_LOW_WATER_STARTUP_IGNORE_MS) {
        product->pump_run_elapsed_ms = sp01_spec_accumulate(
            product->pump_run_elapsed_ms,
            SP01_TICK_MS,
            SP01_SPEC_LOW_WATER_STARTUP_IGNORE_MS);
        product->pump_adc_sum_mv = 0UL;
        product->pump_adc_sample_count = 0U;
        return APP_FALSE;
    }

    product->pump_adc_sum_mv += pump_sense_mv;
    if (product->pump_adc_sample_count < 0xFFFFU) {
        product->pump_adc_sample_count++;
    }

    window_samples = SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS / SP01_TICK_MS;
    if (window_samples == 0UL ||
        product->pump_adc_sample_count < window_samples) {
        return APP_FALSE;
    }

    average_mv = (app_u16)(product->pump_adc_sum_mv /
                           product->pump_adc_sample_count);
    product->pump_adc_last_average_mv = average_mv;
    product->pump_adc_sum_mv = 0UL;
    product->pump_adc_sample_count = 0U;

#if defined(SP01_SPEC_TEST007)
    /* 007：先做预警，再用更低阈值触发真正停泵保护。 */
    if (average_mv <= SP01_SPEC_LOW_WATER_WARNING_THRESHOLD_MV) {
        if (product->low_water_warning_consecutive_count < 0xFFU) {
            product->low_water_warning_consecutive_count++;
        }
        if (product->low_water_warning_consecutive_count >=
            SP01_SPEC_LOW_WATER_WARNING_CONFIRM_COUNT) {
            if (product->low_water_warning_active != APP_TRUE) {
                product->low_water_led_elapsed_ms = 0UL;
            }
            product->low_water_warning_active = APP_TRUE;
        }
    } else if (product->low_water_warning_active == APP_FALSE) {
        product->low_water_warning_consecutive_count = 0U;
    } else if (average_mv > SP01_SPEC_LOW_WATER_WARNING_RECOVERY_MV) {
        product->low_water_warning_consecutive_count = 0U;
        product->low_water_warning_active = APP_FALSE;
        product->low_water_led_elapsed_ms = 0UL;
    }

    if (average_mv <= SP01_SPEC_LOW_WATER_THRESHOLD_MV) {
        if (product->low_water_protection_consecutive_count < 0xFFU) {
            product->low_water_protection_consecutive_count++;
        }
    } else {
        product->low_water_protection_consecutive_count = 0U;
    }

    return product->low_water_protection_consecutive_count >=
           SP01_SPEC_LOW_WATER_CONFIRM_COUNT ? APP_TRUE : APP_FALSE;
#else
    if (average_mv < SP01_SPEC_LOW_WATER_THRESHOLD_MV) {
        if (product->low_water_consecutive_count < 0xFFU) {
            product->low_water_consecutive_count++;
        }
    } else {
        product->low_water_consecutive_count = 0U;
    }

    return product->low_water_consecutive_count >=
           SP01_SPEC_LOW_WATER_CONFIRM_COUNT ? APP_TRUE : APP_FALSE;
#endif
}

/* 低水位报警采用 1Hz 闪烁；每次状态进入锁定时从亮灯相位开始。 */
static app_bool_t sp01_spec_low_water_led_is_on(sp01_spec_product_t *product)
{
    app_bool_t led_is_on;

    led_is_on = product->low_water_led_elapsed_ms <
                SP01_SPEC_LOW_WATER_LED_ON_MS ? APP_TRUE : APP_FALSE;
    product->low_water_led_elapsed_ms += SP01_TICK_MS;
    if (product->low_water_led_elapsed_ms >= SP01_SPEC_LOW_WATER_LED_PERIOD_MS) {
        product->low_water_led_elapsed_ms = 0UL;
    }

    return led_is_on;
}

#if defined(SP01_SPEC_TEST007)
/* 预警灯持续重复单闪：每秒周期内亮约0.1秒。 */
static app_bool_t sp01_spec_low_water_warning_led_is_on(
    sp01_spec_product_t *product)
{
    app_bool_t led_is_on;

    led_is_on = product->low_water_led_elapsed_ms <
                SP01_SPEC_LOW_WATER_WARNING_ON_MS ? APP_TRUE : APP_FALSE;
    product->low_water_led_elapsed_ms += SP01_TICK_MS;
    if (product->low_water_led_elapsed_ms >=
        SP01_SPEC_LOW_WATER_WARNING_PERIOD_MS) {
        product->low_water_led_elapsed_ms = 0UL;
    }
    return led_is_on;
}
#endif
#endif

#if defined(SP01_SPEC_TEST006) || defined(SP01_SPEC_TEST007)
static void sp01_spec_low_water_retry_reset(sp01_spec_product_t *product)
{
    product->low_water_retry_active = APP_FALSE;
    product->low_water_lockout_elapsed_ms = 0UL;
    product->low_water_retry_run_elapsed_ms = 0UL;
    product->low_water_retry_adc_sum_mv = 0UL;
    product->low_water_retry_adc_sample_count = 0U;
    product->low_water_recovery_high_count = 0U;
    product->low_water_retry_schedule_index = 0U;
    product->low_water_retry_count = 0U;
}

#if !defined(SP01_SPEC_TEST007)
static app_u32 sp01_spec_low_water_retry_due_ms(
    const sp01_spec_product_t *product)
{
    if (product->low_water_retry_schedule_index == 0U) {
        return SP01_SPEC_LOW_WATER_RETRY_1_AT_MS;
    }
    if (product->low_water_retry_schedule_index == 1U) {
        return SP01_SPEC_LOW_WATER_RETRY_2_AT_MS;
    }
    if (product->low_water_retry_schedule_index == 2U) {
        return SP01_SPEC_LOW_WATER_RETRY_3_AT_MS;
    }
    if (product->low_water_retry_schedule_index == 3U) {
        return SP01_SPEC_LOW_WATER_RETRY_4_AT_MS;
    }
    if (product->low_water_retry_schedule_index == 4U) {
        return SP01_SPEC_LOW_WATER_RETRY_5_AT_MS;
    }
    return SP01_SPEC_LOW_WATER_RETRY_6_AT_MS;
}
#endif

/*
 * 006 低水位锁定后在 30 秒、2/5/10/20/30 分钟这些绝对时间点试泵；
 * 007 则每隔 30 秒复测一次，持续重试，不自动关机。
 * 每次试泵最多 5 秒，先忽略 1 秒，再按 1 秒窗口采样；一旦连续两个窗口
 * 高于恢复阈值就提前恢复。电池低压时跳过当次时间点，等待下一次 30 秒周期。
 */
static app_bool_t sp01_spec_low_water_retry_update(
    sp01_spec_product_t *product,
    const sp01_spec_product_inputs_t *inputs)
{
    app_u32 window_samples;
    app_u16 average_mv;
#if !defined(SP01_SPEC_TEST007)
    app_u32 retry_due_ms;
#endif
    app_bool_t supply_can_run;

    product->low_water_lockout_elapsed_ms = sp01_spec_accumulate(
        product->low_water_lockout_elapsed_ms,
        SP01_TICK_MS,
        SP01_SPEC_LOW_WATER_LOCKOUT_MAX_MS);

    if (product->low_water_retry_active == APP_FALSE) {
#if defined(SP01_SPEC_TEST007)
        /*
         * 007 使用 lockout_elapsed_ms 作为“距下次试泵”的计时器。
         * 到点先清零，再启动本次试泵，因此后续仍会每隔 30 秒触发，
         * 不会因为 8 位计数器饱和或 30 分钟上限而停止重试。
         */
        if (product->low_water_lockout_elapsed_ms >=
            SP01_SPEC_LOW_WATER_RETRY_INTERVAL_MS) {
            if (product->low_water_retry_schedule_index < 0xFFU) {
                product->low_water_retry_schedule_index++;
            }
            product->low_water_lockout_elapsed_ms = 0UL;
            supply_can_run = (product->supply == SP01_SPEC_SUPPLY_TYPEC ||
                              (product->supply == SP01_SPEC_SUPPLY_BATTERY &&
                               inputs->battery_diag == SP01_POWER_DIAG_NORMAL)) ?
                             APP_TRUE : APP_FALSE;

            if (supply_can_run == APP_TRUE) {
                product->low_water_retry_active = APP_TRUE;
                if (product->low_water_retry_count < 0xFFU) {
                    product->low_water_retry_count++;
                }
                product->low_water_retry_run_elapsed_ms = 0UL;
                product->low_water_retry_adc_sum_mv = 0UL;
                product->low_water_retry_adc_sample_count = 0U;
                product->low_water_recovery_high_count = 0U;
            }
        }
        return APP_FALSE;
#else
        if (product->low_water_retry_schedule_index <
            SP01_SPEC_LOW_WATER_RETRY_COUNT) {
            retry_due_ms = sp01_spec_low_water_retry_due_ms(product);
            if (product->low_water_lockout_elapsed_ms >= retry_due_ms) {
                /* 到点即消费这个时间点，电池低压时跳过，不挪到后面补试。 */
                product->low_water_retry_schedule_index++;
                supply_can_run = (product->supply == SP01_SPEC_SUPPLY_TYPEC ||
                                  (product->supply == SP01_SPEC_SUPPLY_BATTERY &&
                                   inputs->battery_diag == SP01_POWER_DIAG_NORMAL)) ?
                                 APP_TRUE : APP_FALSE;

                if (supply_can_run == APP_TRUE) {
                    product->low_water_retry_active = APP_TRUE;
                    if (product->low_water_retry_count < 0xFFU) {
                        product->low_water_retry_count++;
                    }
                    product->low_water_retry_run_elapsed_ms = 0UL;
                    product->low_water_retry_adc_sum_mv = 0UL;
                    product->low_water_retry_adc_sample_count = 0U;
                    product->low_water_recovery_high_count = 0U;
                    return APP_FALSE;
                }
            }
        }

        /* 第 30 分钟试泵若因电池低压被跳过，或第六次试泵已经失败，关机。 */
        if (product->low_water_lockout_elapsed_ms >=
                SP01_SPEC_LOW_WATER_LOCKOUT_MAX_MS &&
            product->low_water_retry_schedule_index >=
                SP01_SPEC_LOW_WATER_RETRY_COUNT) {
            sp01_spec_power_off(product);
        }
        return APP_FALSE;
#endif
    }

    product->low_water_retry_run_elapsed_ms = sp01_spec_accumulate(
        product->low_water_retry_run_elapsed_ms,
        SP01_TICK_MS,
        SP01_SPEC_LOW_WATER_RETRY_RUN_MS);

    if (product->low_water_retry_run_elapsed_ms >=
        SP01_SPEC_LOW_WATER_RETRY_IGNORE_MS) {
        product->low_water_retry_adc_sum_mv += inputs->pump_sense_mv;
        if (product->low_water_retry_adc_sample_count < 0xFFFFU) {
            product->low_water_retry_adc_sample_count++;
        }

        window_samples = SP01_SPEC_LOW_WATER_AVERAGE_WINDOW_MS / SP01_TICK_MS;
        if (window_samples > 0UL &&
            product->low_water_retry_adc_sample_count >= window_samples) {
            average_mv = (app_u16)(product->low_water_retry_adc_sum_mv /
                                   product->low_water_retry_adc_sample_count);
            product->low_water_retry_adc_sum_mv = 0UL;
            product->low_water_retry_adc_sample_count = 0U;

            if (average_mv > SP01_SPEC_LOW_WATER_RECOVERY_THRESHOLD_MV) {
                if (product->low_water_recovery_high_count < 0xFFU) {
                    product->low_water_recovery_high_count++;
                }
            } else {
                product->low_water_recovery_high_count = 0U;
            }

            if (product->low_water_recovery_high_count >=
                SP01_SPEC_LOW_WATER_RECOVERY_CONFIRM_COUNT) {
                sp01_spec_low_water_retry_reset(product);
                return APP_TRUE;
            }
        }
    }

    if (product->low_water_retry_run_elapsed_ms >=
        SP01_SPEC_LOW_WATER_RETRY_RUN_MS) {
        product->low_water_retry_active = APP_FALSE;
        product->low_water_retry_run_elapsed_ms = 0UL;
        product->low_water_retry_adc_sum_mv = 0UL;
        product->low_water_retry_adc_sample_count = 0U;
        product->low_water_recovery_high_count = 0U;

#if !defined(SP01_SPEC_TEST007)
        if (product->low_water_lockout_elapsed_ms >=
                SP01_SPEC_LOW_WATER_LOCKOUT_MAX_MS &&
            product->low_water_retry_schedule_index >=
                SP01_SPEC_LOW_WATER_RETRY_COUNT) {
            sp01_spec_power_off(product);
        }
#endif
    }

    return APP_FALSE;
}

static void sp01_spec_low_water_recovered(sp01_spec_product_t *product)
{
    product->fault_code = SP01_FAULT_NONE;
    product->low_water_led_elapsed_ms = 0UL;
    sp01_spec_pump_adc_reset(product);

    if (product->supply == SP01_SPEC_SUPPLY_TYPEC) {
        sp01_spec_set_state(product, SP01_SPEC_STATE_TYPEC_CONTINUOUS);
    } else {
        product->fur_flush_due = APP_FALSE;
        product->fur_flush_elapsed_ms = 0UL;
        product->radar_armed = APP_FALSE;
        product->radar_warmup_elapsed_ms = 0UL;
        product->radar_idle_elapsed_ms = 0UL;
        sp01_spec_set_state(product, SP01_SPEC_STATE_BATTERY_READY);
    }
}
#endif

/* CHRG/DONE 只间接代表供电状态，必须持续稳定 1 秒才接受变化。 */
static app_bool_t sp01_spec_supply_update(sp01_spec_product_t *product,
                                          sp01_spec_supply_t observed_supply)
{
    if (observed_supply == product->supply) {
        product->supply_candidate = observed_supply;
        product->supply_candidate_ticks = 0U;
        return APP_FALSE;
    }

    if (observed_supply != product->supply_candidate) {
        product->supply_candidate = observed_supply;
        product->supply_candidate_ticks = 1U;
    } else if (product->supply_candidate_ticks < SP01_SPEC_SUPPLY_DEBOUNCE_TICKS) {
        product->supply_candidate_ticks++;
    }

    if (product->supply_candidate_ticks >= SP01_SPEC_SUPPLY_DEBOUNCE_TICKS) {
        product->supply = product->supply_candidate;
        product->supply_candidate_ticks = 0U;
        return APP_TRUE;
    }

    return APP_FALSE;
}

static void sp01_spec_power_off(sp01_spec_product_t *product)
{
    product->powered_on = APP_FALSE;
    product->night_mode = APP_FALSE;
    product->radar_armed = APP_FALSE;
    product->fur_flush_due = APP_FALSE;
    product->fur_flush_elapsed_ms = 0UL;
    product->radar_warmup_elapsed_ms = 0UL;
    product->radar_idle_elapsed_ms = 0UL;
    product->lockout_remaining_ms = 0UL;
#if defined(SP01_SPEC_TEST006) || defined(SP01_SPEC_TEST007)
    sp01_spec_low_water_retry_reset(product);
#endif
    sp01_spec_pump_adc_reset(product);
    sp01_spec_set_state(product, SP01_SPEC_STATE_OFF);
}

/*
 * Type-C 拔出、但电池仍在供电时，保持产品开机状态并切到电池待机。
 * 不能调用 sp01_spec_power_off()，否则用户必须重新长按才能恢复雷达出水。
 */
static void sp01_spec_switch_to_battery(sp01_spec_product_t *product)
{
    if (product->powered_on != APP_TRUE) {
        sp01_spec_set_state(product, SP01_SPEC_STATE_OFF);
        return;
    }

    product->night_mode = APP_FALSE;
    product->fur_flush_due = APP_FALSE;
    product->fur_flush_elapsed_ms = 0UL;
    product->radar_armed = APP_FALSE;
    product->radar_warmup_elapsed_ms = 0UL;
    product->radar_idle_elapsed_ms = 0UL;
    product->fault_code = SP01_FAULT_NONE;
    sp01_spec_set_state(product, SP01_SPEC_STATE_BATTERY_READY);
}

static app_bool_t sp01_spec_battery_can_run(sp01_power_diag_t diag)
{
    return (diag == SP01_POWER_DIAG_NORMAL ||
            diag == SP01_POWER_DIAG_LOW) ? APP_TRUE : APP_FALSE;
}

static void sp01_spec_radar_update_arming(sp01_spec_product_t *product,
                                          const sp01_spec_product_inputs_t *inputs)
{
#if defined(SP01_SPEC_TEST004) || defined(SP01_SPEC_TEST005) || \
    defined(SP01_SPEC_TEST006) || defined(SP01_SPEC_TEST007)
    /*
     * 雷达单项测试采用 5 秒启动等待和 2 秒持续无人确认。
     * 保持原有 30ms 输入去抖，只延后布防，避免雷达上电/冷却恢复时的电平跳变开泵。
     */
    if (product->radar_warmup_elapsed_ms < SP01_SPEC_RADAR_STARTUP_WARMUP_MS) {
        product->radar_warmup_elapsed_ms = sp01_spec_accumulate(
            product->radar_warmup_elapsed_ms,
            SP01_TICK_MS,
            SP01_SPEC_RADAR_STARTUP_WARMUP_MS);
        product->radar_idle_elapsed_ms = 0UL;
        product->radar_armed = APP_FALSE;
        return;
    }

    if (inputs->radar_active == APP_FALSE) {
        product->radar_idle_elapsed_ms = sp01_spec_accumulate(
            product->radar_idle_elapsed_ms,
            SP01_TICK_MS,
            SP01_SPEC_RADAR_IDLE_ARM_MS);
        if (product->radar_idle_elapsed_ms >= SP01_SPEC_RADAR_IDLE_ARM_MS) {
            product->radar_armed = APP_TRUE;
        }
    } else {
        /* 活动信号未结束前不布防；必须重新连续确认无人 2 秒。 */
        product->radar_idle_elapsed_ms = 0UL;
        /* 已布防时允许本 tick 的新上升沿通过，随后由触发分支撤防。 */
        if (product->radar_armed != APP_TRUE ||
            inputs->radar_rising_edge != APP_TRUE) {
            product->radar_armed = APP_FALSE;
        }
    }
#else
    /* 003 及更早版本保持原有布防时序，便于回退比较。 */
    if (inputs->radar_active == APP_FALSE) {
        product->radar_armed = APP_TRUE;
    }
#endif
}

static void sp01_spec_power_on(sp01_spec_product_t *product,
                               sp01_buzzer_mode_t *buzzer_request)
{
#if defined(SP01_SPEC_TEST006) || defined(SP01_SPEC_TEST007)
    sp01_spec_low_water_retry_reset(product);
#endif
    sp01_spec_pump_adc_reset(product);
    if (product->supply == SP01_SPEC_SUPPLY_TYPEC) {
        product->powered_on = APP_TRUE;
        product->night_mode = APP_FALSE;
        product->fault_code = SP01_FAULT_NONE;
        sp01_spec_set_state(product, SP01_SPEC_STATE_TYPEC_CONTINUOUS);
        *buzzer_request = SP01_BUZZER_POWER_ON;
    } else if (product->supply == SP01_SPEC_SUPPLY_BATTERY) {
        product->powered_on = APP_TRUE;
        product->night_mode = APP_FALSE;
        product->radar_armed = APP_FALSE;
        product->fur_flush_due = APP_FALSE;
        product->fur_flush_elapsed_ms = 0UL;
        product->radar_warmup_elapsed_ms = 0UL;
        product->radar_idle_elapsed_ms = 0UL;
        product->fault_code = SP01_FAULT_NONE;
        sp01_spec_set_state(product, SP01_SPEC_STATE_BATTERY_READY);
        *buzzer_request = SP01_BUZZER_POWER_ON;
    }
}

static sp01_outputs_t sp01_spec_outputs_default(void)
{
    sp01_outputs_t outputs;

    outputs.pump_enable = APP_FALSE;
    outputs.radar_power_enable = APP_FALSE;
    outputs.led_mode = SP01_LED_OFF;
    outputs.buzzer_mode = SP01_BUZZER_OFF;
    outputs.buzzer_frequency_hz = 0U;
    outputs.buzzer_duty_percent = 0U;
    return outputs;
}

sp01_spec_supply_t sp01_spec_detect_supply(app_bool_t charging_active,
                                           app_bool_t charge_done,
                                           app_bool_t charge_conflict,
                                           app_u16 battery_mv)
{
    /*
     * 原理图中 Type-C 的 VUSB 会经 D3 给 BAT 节点供电。
     * 因此无电池插 Type-C 时，PB1/ADC_VBAT 也可能读到约 4.5V；
     * 高于单节锂电池正常电压范围时，应优先判为 Type-C。
     */
    if (battery_mv >= SP01_SPEC_TYPEC_BAT_NODE_MIN_MV) {
        return SP01_SPEC_SUPPLY_TYPEC;
    }

    if (charge_conflict == APP_TRUE) {
        return SP01_SPEC_SUPPLY_UNKNOWN;
    }

    /* CHRG 或 DONE 有效都说明 Type-C 充电器在工作，满电时也能识别。 */
    if (charging_active == APP_TRUE || charge_done == APP_TRUE) {
        return SP01_SPEC_SUPPLY_TYPEC;
    }

    if (battery_mv >= SP01_SPEC_BATTERY_PRESENT_MIN_MV) {
        return SP01_SPEC_SUPPLY_BATTERY;
    }

    return SP01_SPEC_SUPPLY_UNKNOWN;
}

void sp01_spec_product_init(sp01_spec_product_t *product)
{
    if (product == 0) {
        return;
    }

    product->supply = SP01_SPEC_SUPPLY_UNKNOWN;
    product->supply_candidate = SP01_SPEC_SUPPLY_UNKNOWN;
    product->supply_candidate_ticks = 0U;
    product->state = SP01_SPEC_STATE_BOOT;
    sp01_touch_button_init(&product->key_button);
    product->powered_on = APP_FALSE;
    product->night_mode = APP_FALSE;
    product->radar_armed = APP_FALSE;
    product->fur_flush_due = APP_FALSE;
    product->boot_elapsed_ms = 0UL;
    product->state_elapsed_ms = 0UL;
    product->lockout_remaining_ms = 0UL;
    product->fur_flush_elapsed_ms = 0UL;
    product->radar_warmup_elapsed_ms = 0UL;
    product->radar_idle_elapsed_ms = 0UL;
    product->charge_pulse_elapsed_ms = 0UL;
    product->low_water_led_elapsed_ms = 0UL;
    sp01_spec_pump_adc_reset(product);
#if defined(SP01_SPEC_TEST006) || defined(SP01_SPEC_TEST007)
    sp01_spec_low_water_retry_reset(product);
#endif
    product->fault_code = SP01_FAULT_NONE;
}

sp01_outputs_t sp01_spec_product_tick(sp01_spec_product_t *product,
                                      const sp01_spec_product_inputs_t *inputs)
{
    sp01_outputs_t outputs;
    sp01_touch_button_event_t key_events;
    sp01_buzzer_mode_t buzzer_request;
    app_u32 next_elapsed_ms;
    app_bool_t supply_changed;

    outputs = sp01_spec_outputs_default();
    buzzer_request = SP01_BUZZER_OFF;
    if (product == 0 || inputs == 0) {
        return outputs;
    }

    supply_changed = sp01_spec_supply_update(product, inputs->supply);

    key_events = sp01_touch_button_update(&product->key_button,
                                          inputs->key_pressed,
                                          APP_FALSE,
                                          SP01_TICK_MS,
                                          SP01_SPEC_POWER_PRESS_MS);

    if (product->state == SP01_SPEC_STATE_BOOT) {
        product->boot_elapsed_ms = sp01_spec_accumulate(
            product->boot_elapsed_ms,
            SP01_TICK_MS,
            SP01_SPEC_BOOT_SETTLE_MS);
        if (product->boot_elapsed_ms >= SP01_SPEC_BOOT_SETTLE_MS) {
            if (product->supply == SP01_SPEC_SUPPLY_TYPEC) {
                sp01_spec_power_on(product, &buzzer_request);
            } else {
                sp01_spec_set_state(product, SP01_SPEC_STATE_OFF);
            }
        }
        outputs.buzzer_mode = buzzer_request;
        return outputs;
    }

    if (supply_changed == APP_TRUE) {
#if defined(SP01_SPEC_TEST006) || defined(SP01_SPEC_TEST007)
        if (product->state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT &&
            product->supply != SP01_SPEC_SUPPLY_UNKNOWN &&
            product->powered_on == APP_TRUE) {
            /* Type-C/电池切换时保持低水位锁定，不能靠换供电绕过保护。 */
        } else
#endif
        if (product->supply == SP01_SPEC_SUPPLY_TYPEC) {
            /* Type-C 已稳定后依规格开机进入持续出水。 */
            sp01_spec_power_on(product, &buzzer_request);
        } else if (product->supply == SP01_SPEC_SUPPLY_BATTERY &&
                   product->powered_on == APP_TRUE) {
            /* Type-C 拔出但电池仍在：无须重新开机，自动回到电池待机。 */
            sp01_spec_switch_to_battery(product);
        } else {
            /* 拔掉 Type-C 或供電來源不明時先停泵，電池模式需長按開機。 */
            sp01_spec_power_off(product);
        }
    }

    if (product->supply == SP01_SPEC_SUPPLY_BATTERY &&
        product->powered_on == APP_TRUE &&
        sp01_spec_battery_can_run(inputs->battery_diag) == APP_FALSE) {
        product->fault_code = SP01_FAULT_BATTERY_LOW;
        sp01_spec_power_off(product);
    }

    if (key_events.long_edge == APP_TRUE &&
        product->supply != SP01_SPEC_SUPPLY_UNKNOWN) {
        if (product->powered_on == APP_TRUE) {
            sp01_spec_power_off(product);
            buzzer_request = SP01_BUZZER_POWER_OFF;
        } else if (product->supply == SP01_SPEC_SUPPLY_TYPEC ||
                   sp01_spec_battery_can_run(inputs->battery_diag) == APP_TRUE) {
            sp01_spec_power_on(product, &buzzer_request);
        } else {
            buzzer_request = SP01_BUZZER_ERROR;
        }
    } else if (key_events.short_release == APP_TRUE &&
               product->powered_on == APP_TRUE) {
        product->night_mode = product->night_mode == APP_TRUE ?
                              APP_FALSE : APP_TRUE;
        buzzer_request = SP01_BUZZER_KEY_CLICK;
        if (product->night_mode == APP_TRUE) {
            product->fur_flush_elapsed_ms = 0UL;
            product->fur_flush_due = APP_FALSE;
            if (product->state == SP01_SPEC_STATE_FUR_FLUSH) {
                sp01_spec_set_state(product, SP01_SPEC_STATE_BATTERY_READY);
            }
        }
    }

    if (product->powered_on == APP_TRUE &&
        product->supply == SP01_SPEC_SUPPLY_TYPEC) {
        if (product->state != SP01_SPEC_STATE_LOW_WATER_LOCKOUT) {
            product->state = SP01_SPEC_STATE_TYPEC_CONTINUOUS;
        }
    } else if (product->powered_on == APP_TRUE &&
               product->supply == SP01_SPEC_SUPPLY_BATTERY) {
        if (product->night_mode == APP_TRUE) {
            product->fur_flush_elapsed_ms = 0UL;
            product->fur_flush_due = APP_FALSE;
        } else if (product->state == SP01_SPEC_STATE_BATTERY_READY ||
                   product->state == SP01_SPEC_STATE_SENSOR_LOCKOUT) {
            product->fur_flush_elapsed_ms = sp01_spec_accumulate(
                product->fur_flush_elapsed_ms,
                SP01_TICK_MS,
                SP01_SPEC_FUR_FLUSH_INTERVAL_MS);
            if (product->fur_flush_elapsed_ms >= SP01_SPEC_FUR_FLUSH_INTERVAL_MS) {
                product->fur_flush_due = APP_TRUE;
            }
        }

        if (product->state == SP01_SPEC_STATE_BATTERY_READY) {
            if (product->fur_flush_due == APP_TRUE) {
                product->radar_armed = APP_FALSE;
                product->radar_idle_elapsed_ms = 0UL;
                sp01_spec_set_state(product, SP01_SPEC_STATE_FUR_FLUSH);
            } else {
                sp01_spec_radar_update_arming(product, inputs);
                if (product->radar_armed == APP_TRUE &&
                    inputs->radar_rising_edge == APP_TRUE) {
                    product->radar_armed = APP_FALSE;
                    product->radar_idle_elapsed_ms = 0UL;
                    sp01_spec_set_state(product, SP01_SPEC_STATE_SENSOR_RUN);
                }
            }
        } else if (product->state == SP01_SPEC_STATE_SENSOR_RUN) {
            product->radar_armed = APP_FALSE;
            product->radar_idle_elapsed_ms = 0UL;
            next_elapsed_ms = sp01_spec_accumulate(product->state_elapsed_ms,
                                                   SP01_TICK_MS,
                                                   SP01_SPEC_SENSOR_RUN_MS);
            product->state_elapsed_ms = next_elapsed_ms;
            if (next_elapsed_ms >= SP01_SPEC_SENSOR_RUN_MS) {
                product->lockout_remaining_ms = SP01_SPEC_SENSOR_LOCKOUT_MS;
                sp01_spec_set_state(product, SP01_SPEC_STATE_SENSOR_LOCKOUT);
            }
        } else if (product->state == SP01_SPEC_STATE_SENSOR_LOCKOUT) {
            product->radar_armed = APP_FALSE;
            product->radar_idle_elapsed_ms = 0UL;
            if (product->lockout_remaining_ms <= SP01_TICK_MS) {
                product->lockout_remaining_ms = 0UL;
                sp01_spec_set_state(product, SP01_SPEC_STATE_BATTERY_READY);
            } else {
                product->lockout_remaining_ms -= SP01_TICK_MS;
            }
        } else if (product->state == SP01_SPEC_STATE_FUR_FLUSH) {
            if (product->night_mode == APP_TRUE) {
                sp01_spec_set_state(product, SP01_SPEC_STATE_BATTERY_READY);
            } else {
                next_elapsed_ms = sp01_spec_accumulate(product->state_elapsed_ms,
                                                       SP01_TICK_MS,
                                                       SP01_SPEC_FUR_FLUSH_RUN_MS);
                product->state_elapsed_ms = next_elapsed_ms;
                if (next_elapsed_ms >= SP01_SPEC_FUR_FLUSH_RUN_MS) {
                    product->fur_flush_elapsed_ms = 0UL;
                    product->fur_flush_due = APP_FALSE;
                    sp01_spec_set_state(product, SP01_SPEC_STATE_BATTERY_READY);
                }
            }
        }
    }

#if defined(SP01_SPEC_TEST005) || defined(SP01_SPEC_TEST006) || \
    defined(SP01_SPEC_TEST007)
    {
        app_bool_t pump_requested;

        pump_requested = (product->powered_on == APP_TRUE &&
                          ((product->supply == SP01_SPEC_SUPPLY_TYPEC) ||
                           (product->supply == SP01_SPEC_SUPPLY_BATTERY &&
                            (product->state == SP01_SPEC_STATE_SENSOR_RUN ||
                             product->state == SP01_SPEC_STATE_FUR_FLUSH)))) ?
                         APP_TRUE : APP_FALSE;
        if (product->state != SP01_SPEC_STATE_LOW_WATER_LOCKOUT &&
            sp01_spec_low_water_update(product,
                                       pump_requested,
                                       inputs->pump_sense_mv) == APP_TRUE) {
            product->fault_code = SP01_FAULT_LOW_WATER;
            sp01_spec_set_state(product, SP01_SPEC_STATE_LOW_WATER_LOCKOUT);
            buzzer_request = SP01_BUZZER_LOW_WATER;
        }
    }
#endif

#if defined(SP01_SPEC_TEST006) || defined(SP01_SPEC_TEST007)
    if (product->state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT &&
        sp01_spec_low_water_retry_update(product, inputs) == APP_TRUE) {
        sp01_spec_low_water_recovered(product);
    }
#endif

    if (product->state == SP01_SPEC_STATE_LOW_WATER_LOCKOUT) {
        outputs.pump_enable = APP_FALSE;
        outputs.radar_power_enable = APP_FALSE;
#if defined(SP01_SPEC_TEST006) || defined(SP01_SPEC_TEST007)
        outputs.pump_enable = product->low_water_retry_active;
#endif
#if defined(SP01_SPEC_TEST005) || defined(SP01_SPEC_TEST006) || \
    defined(SP01_SPEC_TEST007)
        outputs.led_mode = sp01_spec_low_water_led_is_on(product) == APP_TRUE ?
                           SP01_LED_LOW_WATER : SP01_LED_OFF;
#else
        outputs.led_mode = SP01_LED_LOW_WATER;
#endif
    } else if (product->powered_on == APP_TRUE &&
        product->supply == SP01_SPEC_SUPPLY_TYPEC) {
        outputs.pump_enable = APP_TRUE;
        if (product->night_mode == APP_FALSE) {
            if (inputs->charging_active == APP_TRUE) {
                product->charge_pulse_elapsed_ms =
                    (product->charge_pulse_elapsed_ms + SP01_TICK_MS) %
                    SP01_SPEC_CHARGE_PULSE_PERIOD_MS;
                if (product->charge_pulse_elapsed_ms < SP01_SPEC_CHARGE_PULSE_ON_MS) {
                    outputs.led_mode = SP01_LED_PRODUCT_CHARGING;
                }
            } else if (inputs->charge_done == APP_TRUE) {
                product->charge_pulse_elapsed_ms = 0UL;
                outputs.led_mode = SP01_LED_PRODUCT_FULL;
            } else {
                product->charge_pulse_elapsed_ms = 0UL;
                outputs.led_mode = SP01_LED_PRODUCT_CONTINUOUS;
            }
        }
#if defined(SP01_SPEC_TEST007)
        if (product->low_water_warning_active == APP_TRUE) {
            outputs.led_mode = sp01_spec_low_water_warning_led_is_on(product) ==
                               APP_TRUE ?
                               SP01_LED_PRODUCT_LOW_WATER_WARNING_TYPEC :
                               SP01_LED_OFF;
        }
#endif
    } else if (product->powered_on == APP_TRUE &&
               product->supply == SP01_SPEC_SUPPLY_BATTERY) {
        outputs.radar_power_enable = APP_TRUE;
        /* 电池模式的实际水泵开关是 PB2：board_port 会把 TRUE 转成 PB2 高电平。 */
        outputs.pump_enable = (product->state == SP01_SPEC_STATE_SENSOR_RUN ||
                               product->state == SP01_SPEC_STATE_FUR_FLUSH) ?
                              APP_TRUE : APP_FALSE;
        if (product->night_mode == APP_FALSE) {
            if (inputs->battery_diag == SP01_POWER_DIAG_LOW) {
                product->fault_code = SP01_FAULT_BATTERY_LOW;
                outputs.led_mode = SP01_LED_PRODUCT_LOW_BATTERY;
            } else {
                product->fault_code = SP01_FAULT_NONE;
                outputs.led_mode = SP01_LED_SENSOR_READY;
            }
        }
#if defined(SP01_SPEC_TEST007)
        if (product->low_water_warning_active == APP_TRUE) {
            if (sp01_spec_low_water_warning_led_is_on(product) == APP_TRUE) {
                /* 水位过低单独出现为绿闪；再叠加低电量时改为红闪。 */
                outputs.led_mode =
                    inputs->battery_diag == SP01_POWER_DIAG_LOW ?
                    SP01_LED_PRODUCT_LOW_BATTERY :
                    SP01_LED_PRODUCT_LOW_WATER_WARNING_BATTERY;
            } else {
                outputs.led_mode = SP01_LED_OFF;
            }
        }
#endif
    }

    outputs.buzzer_mode = buzzer_request;
    return outputs;
}

const char *sp01_spec_state_name(sp01_spec_state_t state)
{
    switch (state) {
    case SP01_SPEC_STATE_BOOT:
        return "BOOT";
    case SP01_SPEC_STATE_OFF:
        return "OFF";
    case SP01_SPEC_STATE_BATTERY_READY:
        return "BATTERY_READY";
    case SP01_SPEC_STATE_SENSOR_RUN:
        return "SENSOR_RUN";
    case SP01_SPEC_STATE_SENSOR_LOCKOUT:
        return "SENSOR_LOCKOUT";
    case SP01_SPEC_STATE_FUR_FLUSH:
        return "FUR_FLUSH";
    case SP01_SPEC_STATE_TYPEC_CONTINUOUS:
        return "TYPEC_CONTINUOUS";
    case SP01_SPEC_STATE_LOW_WATER_LOCKOUT:
        return "LOW_WATER_LOCKOUT";
    default:
        return "UNKNOWN";
    }
}
