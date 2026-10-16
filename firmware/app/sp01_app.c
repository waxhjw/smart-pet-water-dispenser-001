#include "sp01_app.h"
#include "app_config.h"

static sp01_outputs_t sp01_outputs_default(void)
{
    sp01_outputs_t outputs;

    outputs.pump_enable = APP_FALSE;
    /* 默认先关闭雷达供电，具体状态确认后再打开。 */
    outputs.radar_power_enable = APP_FALSE;
    outputs.led_mode = SP01_LED_STANDBY;
    outputs.buzzer_mode = SP01_BUZZER_OFF;
    outputs.buzzer_frequency_hz = 0U;
    outputs.buzzer_duty_percent = 0U;

    return outputs;
}

static void sp01_enter_state(sp01_app_t *app, sp01_state_t next)
{
    app->state = next;
    app->state_elapsed_ms = 0;
}

static void sp01_enter_fault(sp01_app_t *app, sp01_fault_code_t fault_code)
{
    app->fault_code = fault_code;
    app->fault_ack_armed = APP_FALSE;
    sp01_enter_state(app, SP01_STATE_FAULT);
}

static void sp01_enter_low_water_path(sp01_app_t *app)
{
    app->fault_code = SP01_FAULT_LOW_WATER;

    if (app->low_water_confirm_count < 255U) {
        app->low_water_confirm_count++;
    }

    if (app->low_water_confirm_count >= SP01_LOW_WATER_CONFIRM_COUNT) {
        sp01_enter_state(app, SP01_STATE_LOW_WATER_ALARM);
    } else {
        sp01_enter_state(app, SP01_STATE_LOW_WATER_SUSPECT);
    }
}

static void sp01_apply_charge_led(const sp01_inputs_t *inputs, sp01_outputs_t *outputs)
{
    /*
     * 充电显示优先级低于缺水/故障，高于普通待机。
     * 这样低水位红灯不会被充电灯覆盖。
     */
    if (outputs->led_mode == SP01_LED_STANDBY) {
        if (inputs->charge_done == APP_TRUE) {
            outputs->led_mode = SP01_LED_FULL;
        } else if (inputs->charging_active == APP_TRUE) {
            outputs->led_mode = SP01_LED_CHARGING;
        }
    }
}

void sp01_app_init(sp01_app_t *app)
{
    app->state = SP01_STATE_BOOT;
    app->state_elapsed_ms = 0;
    app->lockout_remaining_ms = 0;
    app->low_water_confirm_count = 0;
    app->night_mode_enabled = APP_FALSE;
    app->radar_armed = APP_FALSE;
    app->fault_ack_armed = APP_FALSE;
    sp01_touch_button_init(&app->key_button);
    app->fault_code = SP01_FAULT_NONE;
}

sp01_outputs_t sp01_app_tick(sp01_app_t *app, const sp01_inputs_t *inputs)
{
    sp01_outputs_t outputs;
    sp01_touch_button_event_t key_events;

    outputs = sp01_outputs_default();
    key_events = sp01_touch_button_update(&app->key_button,
                                          inputs->key_pressed,
                                          inputs->key_long_pressed,
                                          SP01_TICK_MS,
                                          SP01_KEY_LONG_PRESS_MS);

    app->state_elapsed_ms += SP01_TICK_MS;

    if (key_events.long_edge == APP_TRUE &&
        app->state != SP01_STATE_FAULT &&
        app->state != SP01_STATE_LOW_WATER_ALARM) {
        app->night_mode_enabled = (app->night_mode_enabled == APP_TRUE) ? APP_FALSE : APP_TRUE;
        outputs.buzzer_mode = SP01_BUZZER_KEY_CLICK;
    }

    /*
     * 运行中检测到低压，先停泵再回到待机保护态，不能等当前运行流程自然结束。
     */
    if (inputs->battery_low == APP_TRUE &&
        (app->state == SP01_STATE_SENSOR_RUN ||
         app->state == SP01_STATE_MANUAL_RUN)) {
        app->fault_code = SP01_FAULT_BATTERY_LOW;
        sp01_enter_state(app, SP01_STATE_STANDBY);
    }

    if (app->state != SP01_STATE_STANDBY &&
        inputs->radar_active == APP_TRUE) {
        app->radar_armed = APP_FALSE;
    }

    switch (app->state) {
    case SP01_STATE_BOOT:
        /* 上电阶段默认关泵，等电源/传感器稳定后再进入待机。 */
        outputs.pump_enable = APP_FALSE;
        outputs.led_mode = SP01_LED_OFF;
        if (app->state_elapsed_ms >= SP01_BOOT_SETTLE_MS) {
            sp01_enter_state(app, SP01_STATE_STANDBY);
        }
        break;

    case SP01_STATE_STANDBY:
        outputs.pump_enable = APP_FALSE;
        outputs.led_mode = app->night_mode_enabled == APP_TRUE ? SP01_LED_OFF : SP01_LED_STANDBY;

        if (inputs->battery_low == APP_TRUE) {
            app->fault_code = SP01_FAULT_BATTERY_LOW;
            outputs.led_mode = SP01_LED_FAULT;
        } else {
            if (app->fault_code == SP01_FAULT_BATTERY_LOW) {
                app->fault_code = SP01_FAULT_NONE;
            }

            if (key_events.short_release == APP_TRUE) {
                outputs.buzzer_mode = SP01_BUZZER_KEY_CLICK;
                app->low_water_confirm_count = 0;
                app->fault_code = SP01_FAULT_NONE;
                sp01_enter_state(app, SP01_STATE_MANUAL_RUN);
            } else if (inputs->radar_active == APP_FALSE) {
                app->radar_armed = APP_TRUE;
            } else if (app->radar_armed == APP_TRUE) {
                app->radar_armed = APP_FALSE;
                app->low_water_confirm_count = 0;
                app->fault_code = SP01_FAULT_NONE;
                sp01_enter_state(app, SP01_STATE_SENSOR_RUN);
            }
        }
        break;

    case SP01_STATE_SENSOR_RUN:
        outputs.pump_enable = APP_TRUE;
        outputs.led_mode = app->night_mode_enabled == APP_TRUE ? SP01_LED_OFF : SP01_LED_PUMPING;

        if (inputs->pump_diag == SP01_PUMP_DIAG_LOW_WATER_SUSPECT ||
            inputs->pump_diag == SP01_PUMP_DIAG_NO_LOAD) {
            sp01_enter_low_water_path(app);
        } else if (inputs->pump_diag == SP01_PUMP_DIAG_OVER_CURRENT) {
            sp01_enter_fault(app, SP01_FAULT_PUMP_OVER_CURRENT);
        } else if (app->state_elapsed_ms >= SP01_SENSOR_RUN_MS) {
            app->lockout_remaining_ms = SP01_SENSOR_LOCKOUT_MS;
            sp01_enter_state(app, SP01_STATE_LOCKOUT);
        }
        break;

    case SP01_STATE_MANUAL_RUN:
        outputs.pump_enable = APP_TRUE;
        outputs.led_mode = app->night_mode_enabled == APP_TRUE ? SP01_LED_OFF : SP01_LED_PUMPING;

        if (key_events.pressed_edge == APP_TRUE) {
            outputs.buzzer_mode = SP01_BUZZER_KEY_CLICK;
            sp01_enter_state(app, SP01_STATE_STANDBY);
        } else if (inputs->pump_diag == SP01_PUMP_DIAG_LOW_WATER_SUSPECT ||
                   inputs->pump_diag == SP01_PUMP_DIAG_NO_LOAD) {
            sp01_enter_low_water_path(app);
        } else if (inputs->pump_diag == SP01_PUMP_DIAG_OVER_CURRENT) {
            sp01_enter_fault(app, SP01_FAULT_PUMP_OVER_CURRENT);
        } else if (app->state_elapsed_ms >= SP01_MANUAL_RUN_MAX_MS) {
            sp01_enter_state(app, SP01_STATE_STANDBY);
        }
        break;

    case SP01_STATE_LOCKOUT:
        outputs.pump_enable = APP_FALSE;
        outputs.led_mode = app->night_mode_enabled == APP_TRUE ? SP01_LED_OFF : SP01_LED_STANDBY;

        if (key_events.short_release == APP_TRUE) {
            outputs.buzzer_mode = SP01_BUZZER_KEY_CLICK;
            sp01_enter_state(app, SP01_STATE_MANUAL_RUN);
        } else if (app->lockout_remaining_ms <= SP01_TICK_MS) {
            app->lockout_remaining_ms = 0;
            sp01_enter_state(app, SP01_STATE_STANDBY);
        } else {
            app->lockout_remaining_ms -= SP01_TICK_MS;
        }
        break;

    case SP01_STATE_LOW_WATER_SUSPECT:
        /*
         * 疑似缺水先停泵等待，再允许复测。连续确认后才进入红灯锁定。
         * 这样能减少线圈偏移、电源纹波导致的一次性误判。
         */
        outputs.pump_enable = APP_FALSE;
        outputs.led_mode = SP01_LED_LOW_WATER;
        if (app->state_elapsed_ms >= SP01_LOW_WATER_RETRY_OFF_MS) {
            app->fault_code = SP01_FAULT_NONE;
            sp01_enter_state(app, SP01_STATE_SENSOR_RUN);
        }
        break;

    case SP01_STATE_LOW_WATER_ALARM:
        outputs.pump_enable = APP_FALSE;
        outputs.led_mode = SP01_LED_LOW_WATER;
        if (app->state_elapsed_ms <= SP01_TICK_MS) {
            outputs.buzzer_mode = SP01_BUZZER_LOW_WATER;
        }

        if (key_events.pressed_edge == APP_TRUE) {
            outputs.buzzer_mode = SP01_BUZZER_KEY_CLICK;
            app->low_water_confirm_count = 0;
            app->fault_code = SP01_FAULT_NONE;
            sp01_enter_state(app, SP01_STATE_STANDBY);
        }
        break;

    case SP01_STATE_FAULT:
        outputs.pump_enable = APP_FALSE;
        outputs.led_mode = SP01_LED_FAULT;
        outputs.buzzer_mode = SP01_BUZZER_FAULT;

        if (key_events.pressed_edge == APP_TRUE) {
            app->fault_ack_armed = APP_TRUE;
        }
        if (key_events.short_release == APP_TRUE) {
            app->fault_ack_armed = APP_FALSE;
        }
        if (key_events.long_edge == APP_TRUE) {
            if (app->fault_ack_armed == APP_TRUE &&
                inputs->battery_low == APP_FALSE) {
                outputs.buzzer_mode = SP01_BUZZER_KEY_CLICK;
                app->fault_code = SP01_FAULT_NONE;
                sp01_enter_state(app, SP01_STATE_STANDBY);
            }
            app->fault_ack_armed = APP_FALSE;
        }
        break;

    default:
        sp01_enter_fault(app, SP01_FAULT_INVALID_STATE);
        outputs.pump_enable = APP_FALSE;
        outputs.led_mode = SP01_LED_FAULT;
        break;
    }

    /*
     * 状态切换可能发生在本次 tick 内（例如过流、缺水、锁定或手动停止）。
     * 以切换后的状态再次裁剪最终输出，避免出现“状态已变化但执行器晚一拍”。
     */
    switch (app->state) {
    case SP01_STATE_BOOT:
        outputs.pump_enable = APP_FALSE;
        outputs.led_mode = SP01_LED_OFF;
        break;
    case SP01_STATE_STANDBY:
        outputs.pump_enable = APP_FALSE;
        outputs.led_mode = app->fault_code == SP01_FAULT_BATTERY_LOW ?
                           SP01_LED_FAULT :
                           (app->night_mode_enabled == APP_TRUE ?
                            SP01_LED_OFF : SP01_LED_STANDBY);
        break;
    case SP01_STATE_LOCKOUT:
        outputs.pump_enable = APP_FALSE;
        outputs.led_mode = app->night_mode_enabled == APP_TRUE ?
                           SP01_LED_OFF : SP01_LED_STANDBY;
        break;
    case SP01_STATE_LOW_WATER_SUSPECT:
        outputs.pump_enable = APP_FALSE;
        outputs.led_mode = SP01_LED_LOW_WATER;
        break;
    case SP01_STATE_LOW_WATER_ALARM:
        outputs.pump_enable = APP_FALSE;
        outputs.led_mode = SP01_LED_LOW_WATER;
        if (app->state_elapsed_ms <= SP01_TICK_MS) {
            outputs.buzzer_mode = SP01_BUZZER_LOW_WATER;
        }
        break;
    case SP01_STATE_FAULT:
        outputs.pump_enable = APP_FALSE;
        outputs.led_mode = SP01_LED_FAULT;
        outputs.buzzer_mode = SP01_BUZZER_FAULT;
        break;
    case SP01_STATE_SENSOR_RUN:
    case SP01_STATE_MANUAL_RUN:
    default:
        break;
    }

    /*
     * 雷达供电的安全策略：BOOT 和 FAULT 阶段关闭，正常产品状态打开。
     * 这样上电、异常恢复和看门狗复位不会默认给外部模块供电。
     */
    outputs.radar_power_enable = (app->state == SP01_STATE_BOOT ||
                                  app->state == SP01_STATE_FAULT) ?
                                 APP_FALSE : APP_TRUE;

    sp01_apply_charge_led(inputs, &outputs);

    return outputs;
}

const char *sp01_state_name(sp01_state_t state)
{
    switch (state) {
    case SP01_STATE_BOOT:
        return "BOOT";
    case SP01_STATE_STANDBY:
        return "STANDBY";
    case SP01_STATE_SENSOR_RUN:
        return "SENSOR_RUN";
    case SP01_STATE_MANUAL_RUN:
        return "MANUAL_RUN";
    case SP01_STATE_LOCKOUT:
        return "LOCKOUT";
    case SP01_STATE_LOW_WATER_SUSPECT:
        return "LOW_WATER_SUSPECT";
    case SP01_STATE_LOW_WATER_ALARM:
        return "LOW_WATER_ALARM";
    case SP01_STATE_FAULT:
        return "FAULT";
    default:
        return "UNKNOWN";
    }
}

const char *sp01_pump_diag_name(sp01_pump_diag_t diag)
{
    switch (diag) {
    case SP01_PUMP_DIAG_UNKNOWN:
        return "UNKNOWN";
    case SP01_PUMP_DIAG_NORMAL:
        return "NORMAL";
    case SP01_PUMP_DIAG_LOW_WATER_SUSPECT:
        return "LOW_WATER_SUSPECT";
    case SP01_PUMP_DIAG_NO_LOAD:
        return "NO_LOAD";
    case SP01_PUMP_DIAG_OVER_CURRENT:
        return "OVER_CURRENT";
    default:
        return "INVALID";
    }
}

const char *sp01_fault_name(sp01_fault_code_t fault_code)
{
    switch (fault_code) {
    case SP01_FAULT_NONE:
        return "NONE";
    case SP01_FAULT_BATTERY_LOW:
        return "BATTERY_LOW";
    case SP01_FAULT_PUMP_OVER_CURRENT:
        return "PUMP_OVER_CURRENT";
    case SP01_FAULT_LOW_WATER:
        return "LOW_WATER";
    case SP01_FAULT_INVALID_STATE:
        return "INVALID_STATE";
    default:
        return "INVALID";
    }
}
