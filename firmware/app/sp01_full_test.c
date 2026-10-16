#include "sp01_full_test.h"
#include "app_config.h"

static app_u32 sp01_full_accumulate(app_u32 value,
                                    app_u32 tick_ms,
                                    app_u32 limit_ms)
{
    if (value >= limit_ms || tick_ms >= (limit_ms - value)) {
        return limit_ms;
    }
    return value + tick_ms;
}

static void sp01_full_request_buzzer(sp01_full_test_t *test,
                                     sp01_buzzer_mode_t mode)
{
    if (test != 0) {
        test->buzzer_request = mode;
    }
}

static void sp01_full_power_off(sp01_full_test_t *test)
{
    test->power_state = SP01_FULL_POWER_OFF;
    test->night_mode_enabled = APP_FALSE;
    test->radar_armed = APP_FALSE;
    test->pump_running = APP_FALSE;
    test->pump_from_radar = APP_FALSE;
    test->radar_cooldown_ms = 0UL;
    sp01_full_request_buzzer(test, SP01_BUZZER_POWER_OFF);
}

static void sp01_full_power_on(sp01_full_test_t *test)
{
    test->power_state = SP01_FULL_POWER_ON_SENSOR;
    test->night_mode_enabled = APP_FALSE;
    test->radar_armed = APP_FALSE;
    test->pump_running = APP_FALSE;
    test->pump_from_radar = APP_FALSE;
    test->radar_cooldown_ms = 0UL;
    test->low_water_active = APP_FALSE;
    test->low_water_beep_pending = APP_FALSE;
    test->low_water_beeped = APP_FALSE;
    test->fault_active = APP_FALSE;
    test->fault_code = SP01_FAULT_NONE;
#ifdef SP01_LOW_WATER_TEST
    /* 低水位专项版上电静音，避免和低水位报警混淆。 */
    sp01_full_request_buzzer(test, SP01_BUZZER_OFF);
#else
    sp01_full_request_buzzer(test, SP01_BUZZER_POWER_ON);
#endif
}

static void sp01_full_start_pump(sp01_full_test_t *test,
                                 app_bool_t from_radar)
{
    test->pump_running = APP_TRUE;
    test->pump_from_radar = from_radar;
    test->pump_elapsed_ms = 0UL;
}

static void sp01_full_execute_single_click(sp01_full_test_t *test)
{
    if (test->low_water_active == APP_TRUE) {
        /* 低水位锁定后，短按只提示，不允许再次干转水泵。 */
        sp01_full_request_buzzer(test, SP01_BUZZER_LOW_WATER);
    } else if (test->power_state == SP01_FULL_POWER_ON_SENSOR) {
        sp01_full_start_pump(test, APP_FALSE);
        sp01_full_request_buzzer(test, SP01_BUZZER_KEY_CLICK);
    } else if (test->power_state == SP01_FULL_POWER_ON_CONTINUOUS) {
        /* 连续出水已开启，普通短按不改变模式，但仍给出反馈。 */
        sp01_full_request_buzzer(test, SP01_BUZZER_KEY_CLICK);
    } else {
        sp01_full_request_buzzer(test, SP01_BUZZER_ERROR);
    }
}

static void sp01_full_execute_double_click(sp01_full_test_t *test)
{
    if (test->low_water_active == APP_TRUE) {
        /* 低水位锁定后，连续出水模式也必须被拦截。 */
        sp01_full_request_buzzer(test, SP01_BUZZER_LOW_WATER);
    } else if (test->power_state == SP01_FULL_POWER_ON_SENSOR) {
        test->power_state = SP01_FULL_POWER_ON_CONTINUOUS;
        test->night_mode_enabled = APP_FALSE;
        test->pump_running = APP_TRUE;
        test->pump_from_radar = APP_FALSE;
        test->pump_elapsed_ms = 0UL;
        sp01_full_request_buzzer(test, SP01_BUZZER_DOUBLE_CLICK);
    } else if (test->power_state == SP01_FULL_POWER_ON_CONTINUOUS) {
        test->power_state = SP01_FULL_POWER_ON_SENSOR;
        test->pump_running = APP_FALSE;
        test->pump_from_radar = APP_FALSE;
        test->pump_elapsed_ms = 0UL;
        sp01_full_request_buzzer(test, SP01_BUZZER_DOUBLE_CLICK);
    } else {
        sp01_full_request_buzzer(test, SP01_BUZZER_ERROR);
    }
}

static void sp01_full_process_key_release(sp01_full_test_t *test,
                                          app_u32 duration_ms)
{
    if (duration_ms >= SP01_FULL_TEST_NIGHT_PRESS_MS) {
        if (test->power_state == SP01_FULL_POWER_ON_SENSOR) {
            test->night_mode_enabled = test->night_mode_enabled == APP_TRUE ?
                                       APP_FALSE : APP_TRUE;
            sp01_full_request_buzzer(test, SP01_BUZZER_NIGHT);
        } else if (test->power_state == SP01_FULL_POWER_BOOT ||
                   test->power_state == SP01_FULL_POWER_OFF) {
            /* 关机状态长按 5 秒只负责开机，不能误切夜间模式。 */
            sp01_full_power_on(test);
        } else {
            /* 连续出水和夜间模式互斥。 */
            sp01_full_request_buzzer(test, SP01_BUZZER_ERROR);
        }
        return;
    }

    if (duration_ms >= SP01_FULL_TEST_POWER_PRESS_MS) {
        if (test->power_state == SP01_FULL_POWER_OFF) {
            sp01_full_power_on(test);
        } else if (test->power_state == SP01_FULL_POWER_ON_SENSOR ||
                   test->power_state == SP01_FULL_POWER_ON_CONTINUOUS) {
            sp01_full_power_off(test);
        }
        return;
    }

    if (duration_ms > 0UL) {
        if (test->pending_short_click == APP_TRUE &&
            test->pending_click_elapsed_ms <= SP01_FULL_TEST_DOUBLE_CLICK_MS) {
            test->pending_short_click = APP_FALSE;
            test->pending_click_elapsed_ms = 0UL;
            sp01_full_execute_double_click(test);
        } else {
            test->pending_short_click = APP_TRUE;
            test->pending_click_elapsed_ms = 0UL;
        }
    }
}

static void sp01_full_update_pump(sp01_full_test_t *test)
{
    if (test->low_water_active == APP_TRUE ||
        test->fault_active == APP_TRUE) {
        /* 诊断锁定优先于连续出水模式，禁止内部状态重新拉起水泵。 */
        test->pump_running = APP_FALSE;
        test->pump_from_radar = APP_FALSE;
        test->pump_elapsed_ms = 0UL;
        return;
    }

    if (test->power_state == SP01_FULL_POWER_ON_CONTINUOUS) {
        test->pump_running = APP_TRUE;
        test->pump_from_radar = APP_FALSE;
        test->pump_elapsed_ms = 0UL;
        return;
    }

    if (test->pump_running == APP_FALSE) {
        return;
    }

    test->pump_elapsed_ms = sp01_full_accumulate(test->pump_elapsed_ms,
                                                 SP01_TICK_MS,
                                                 SP01_FULL_TEST_PUMP_RUN_MS);
    if (test->pump_elapsed_ms >= SP01_FULL_TEST_PUMP_RUN_MS) {
        test->pump_running = APP_FALSE;
        if (test->pump_from_radar == APP_TRUE) {
            test->radar_cooldown_ms = SP01_FULL_TEST_RADAR_COOLDOWN_MS;
        }
        test->pump_from_radar = APP_FALSE;
    }
}

static void sp01_full_update_timers(sp01_full_test_t *test)
{
    if (test->radar_cooldown_ms > 0UL) {
        if (test->radar_cooldown_ms <= SP01_TICK_MS) {
            test->radar_cooldown_ms = 0UL;
        } else {
            test->radar_cooldown_ms -= SP01_TICK_MS;
        }
    }

    if (test->pending_short_click == APP_TRUE) {
        test->pending_click_elapsed_ms = sp01_full_accumulate(
            test->pending_click_elapsed_ms,
            SP01_TICK_MS,
            SP01_FULL_TEST_DOUBLE_CLICK_MS);
        if (test->pending_click_elapsed_ms >= SP01_FULL_TEST_DOUBLE_CLICK_MS) {
            test->pending_short_click = APP_FALSE;
            test->pending_click_elapsed_ms = 0UL;
            sp01_full_execute_single_click(test);
        }
    }
}

static sp01_outputs_t sp01_full_outputs(const sp01_full_test_t *test)
{
    sp01_outputs_t outputs;

    outputs.pump_enable = APP_FALSE;
    outputs.radar_power_enable = APP_FALSE;
    outputs.led_mode = SP01_LED_OFF;
    outputs.buzzer_mode = test->buzzer_request;
    outputs.buzzer_frequency_hz = 0U;
    outputs.buzzer_duty_percent = 0U;

    if (test->power_state == SP01_FULL_POWER_BOOT ||
        test->power_state == SP01_FULL_POWER_OFF) {
        return outputs;
    }

    outputs.radar_power_enable = APP_TRUE;
    if (test->fault_active == APP_TRUE) {
        outputs.led_mode = SP01_LED_FAULT;
        outputs.buzzer_mode = test->buzzer_request == SP01_BUZZER_OFF ?
                              SP01_BUZZER_FAULT : test->buzzer_request;
        return outputs;
    }

    if (test->low_water_active == APP_TRUE) {
        outputs.led_mode = SP01_LED_LOW_WATER;
        /* 低水位只在首次命中时提示一次，避免每个 tick 重启蜂鸣节奏。 */
        outputs.buzzer_mode = test->buzzer_request;
        return outputs;
    }

    if (test->power_state == SP01_FULL_POWER_ON_CONTINUOUS) {
        outputs.pump_enable = APP_TRUE;
        outputs.led_mode = SP01_LED_CONTINUOUS;
        return outputs;
    }

    outputs.pump_enable = test->pump_running;
    if (test->night_mode_enabled == APP_TRUE) {
        outputs.led_mode = SP01_LED_OFF;
    } else if (test->pump_running == APP_TRUE) {
        /* 整机测试中普通出水固定显示绿灯，便于肉眼判断 15 秒状态。 */
        outputs.led_mode = SP01_LED_SENSOR_PUMPING;
    } else {
        outputs.led_mode = SP01_LED_SENSOR_READY;
    }

    return outputs;
}

void sp01_full_test_init(sp01_full_test_t *test)
{
    if (test == 0) {
        return;
    }

    test->power_state = SP01_FULL_POWER_BOOT;
    sp01_touch_button_init(&test->key_button);
    test->night_mode_enabled = APP_FALSE;
    test->radar_armed = APP_FALSE;
    test->pump_running = APP_FALSE;
    test->pump_from_radar = APP_FALSE;
    test->pending_short_click = APP_FALSE;
    test->low_water_active = APP_FALSE;
    test->low_water_beep_pending = APP_FALSE;
    test->low_water_beeped = APP_FALSE;
    test->fault_active = APP_FALSE;
    test->state_elapsed_ms = 0UL;
    test->pump_elapsed_ms = 0UL;
    test->radar_cooldown_ms = 0UL;
    test->pending_click_elapsed_ms = 0UL;
    test->fault_code = SP01_FAULT_NONE;
    test->buzzer_request = SP01_BUZZER_OFF;
    test->observed_pump_diag = SP01_PUMP_DIAG_UNKNOWN;
}

sp01_outputs_t sp01_full_test_tick(sp01_full_test_t *test,
                                   const sp01_full_test_inputs_t *inputs)
{
    sp01_touch_button_event_t key_event;

    if (test == 0 || inputs == 0) {
        sp01_outputs_t safe_outputs;

        safe_outputs.pump_enable = APP_FALSE;
        safe_outputs.radar_power_enable = APP_FALSE;
        safe_outputs.led_mode = SP01_LED_OFF;
        safe_outputs.buzzer_mode = SP01_BUZZER_OFF;
        safe_outputs.buzzer_frequency_hz = 0U;
        safe_outputs.buzzer_duty_percent = 0U;
        return safe_outputs;
    }

    test->buzzer_request = SP01_BUZZER_OFF;
    test->observed_pump_diag = inputs->pump_diag;
    test->state_elapsed_ms += SP01_TICK_MS;

    /* 先确认水泵已经停止，再发低水位提示，避免报警和出水声重叠。 */
    if (test->low_water_beep_pending == APP_TRUE) {
        test->low_water_beep_pending = APP_FALSE;
        if (test->low_water_beeped == APP_FALSE) {
            test->low_water_beeped = APP_TRUE;
            sp01_full_request_buzzer(test, SP01_BUZZER_LOW_WATER);
        }
    }

    key_event = sp01_touch_button_update(&test->key_button,
                                         inputs->key_pressed,
                                         APP_FALSE,
                                         SP01_TICK_MS,
                                         SP01_FULL_TEST_NIGHT_PRESS_MS);

    if (test->power_state == SP01_FULL_POWER_BOOT &&
        test->state_elapsed_ms >= SP01_FULL_TEST_BOOT_SETTLE_MS) {
        sp01_full_power_on(test);
    }

    sp01_full_update_timers(test);

    if (key_event.released_duration_ms > 0UL &&
        test->power_state != SP01_FULL_POWER_BOOT) {
        sp01_full_process_key_release(test, key_event.released_duration_ms);
    }

    if (inputs->injected_fault == 1U ||
        inputs->pump_diag == SP01_PUMP_DIAG_LOW_WATER_SUSPECT ||
        inputs->pump_diag == SP01_PUMP_DIAG_NO_LOAD) {
        /* 诊断命中后立即停泵并锁定，避免下一 tick 又重新干转。 */
        test->low_water_active = APP_TRUE;
        test->pump_running = APP_FALSE;
        test->pump_from_radar = APP_FALSE;
        test->pump_elapsed_ms = 0UL;
        test->fault_code = SP01_FAULT_LOW_WATER;
        test->low_water_beep_pending = APP_TRUE;
    }

    if (inputs->injected_fault == 2U ||
        inputs->pump_diag == SP01_PUMP_DIAG_OVER_CURRENT) {
        test->fault_active = APP_TRUE;
        test->fault_code = SP01_FAULT_PUMP_OVER_CURRENT;
        test->pump_running = APP_FALSE;
        sp01_full_request_buzzer(test, SP01_BUZZER_FAULT);
    }

    if (test->power_state == SP01_FULL_POWER_ON_SENSOR &&
        inputs->radar_active == APP_FALSE) {
        test->radar_armed = APP_TRUE;
    }

    if (test->power_state == SP01_FULL_POWER_ON_SENSOR &&
        inputs->radar_rising_edge == APP_TRUE &&
        test->radar_armed == APP_TRUE &&
        test->radar_cooldown_ms == 0UL &&
        test->pump_running == APP_FALSE &&
        test->low_water_active == APP_FALSE &&
        test->fault_active == APP_FALSE) {
        sp01_full_start_pump(test, APP_TRUE);
        test->radar_armed = APP_FALSE;
        sp01_full_request_buzzer(test, SP01_BUZZER_RADAR_TEST);
    }

    sp01_full_update_pump(test);
    return sp01_full_outputs(test);
}

const char *sp01_full_power_state_name(sp01_full_power_state_t state)
{
    switch (state) {
    case SP01_FULL_POWER_BOOT:
        return "BOOT";
    case SP01_FULL_POWER_OFF:
        return "POWER_OFF";
    case SP01_FULL_POWER_ON_SENSOR:
        return "POWER_ON_SENSOR";
    case SP01_FULL_POWER_ON_CONTINUOUS:
        return "POWER_ON_CONTINUOUS";
    default:
        return "UNKNOWN";
    }
}
