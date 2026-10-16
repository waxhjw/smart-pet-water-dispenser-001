#include "board_port.h"
#include "../../app/board_profile.h"
#include "../../app/sp01_indicator.h"

#ifndef HOST_SIM
#include "board_port_vendor.h"
#ifndef SP01_PY32_VENDOR_PORT
#error "Define SP01_PY32_VENDOR_PORT and add a real vendor_port.c before building target firmware."
#endif
#endif

static volatile app_u8 g_tick_10ms_ready;

#ifdef HOST_SIM
static board_inputs_t g_sim_inputs;
static sp01_outputs_t g_sim_outputs;
static board_reset_reason_t g_sim_reset_reason;
#endif

static app_bool_t board_effective_pump_enable(app_bool_t requested)
{
#if defined(SP01_BENCH_NO_PUMP) || defined(SP01_RGB_TUNER_TEST)
    (void)requested;
    return APP_FALSE;
#else
    return requested;
#endif
}

void board_init(void)
{
    /*
     * 安全前提：初始化期间所有执行器保持关闭。
     * 真实 PY32 GPIO/ADC/Timer 初始化等拿到 SDK 后在这里补。
     */
    g_tick_10ms_ready = 0U;
#ifndef HOST_SIM
    board_vendor_init();
#else
    g_sim_reset_reason = BOARD_RESET_REASON_UNKNOWN;
#endif
    board_safe_outputs_off();
}

void board_watchdog_refresh(void)
{
#ifndef HOST_SIM
    board_vendor_watchdog_refresh();
#endif
}

board_reset_reason_t board_get_reset_reason(void)
{
#ifdef HOST_SIM
    return g_sim_reset_reason;
#else
    return board_vendor_get_reset_reason() == BOARD_VENDOR_RESET_IWDG ?
           BOARD_RESET_REASON_IWDG : BOARD_RESET_REASON_UNKNOWN;
#endif
}

#ifndef HOST_SIM
static app_bool_t board_output_level(app_bool_t enabled, app_u8 active_level)
{
    return enabled == APP_TRUE ?
           (active_level == BOARD_GPIO_ACTIVE_HIGH ? APP_TRUE : APP_FALSE) :
           (active_level == BOARD_GPIO_ACTIVE_HIGH ? APP_FALSE : APP_TRUE);
}

static void board_set_indicators(sp01_led_mode_t led_mode,
                                 sp01_buzzer_mode_t buzzer_mode,
                                 app_u16 buzzer_frequency_hz,
                                 app_u8 buzzer_duty_percent)
{
#if defined(SP01_RGB_TUNER_TEST)
    /* RGB 由 SysTick 软件 PWM 直接驱动；泵、雷达和蜂鸣器保持关闭。 */
    board_vendor_buzzer_apply(APP_FALSE, 0U, 0U);
    (void)led_mode;
    (void)buzzer_mode;
    (void)buzzer_frequency_hz;
    (void)buzzer_duty_percent;
    return;
#elif defined(SP01_RGB_WHITE_BALANCE_TEST) || defined(SP01_SPEC_TEST007)
    /* 白平衡测试和007共用 RGB 软件 PWM，不能再对灯脚做静态 GPIO 写入。 */
    app_u8 red_duty = 0U;
    app_u8 green_duty = 0U;
    app_u8 blue_duty = 0U;

#if defined(SP01_SPEC_TEST007)
    /* 007：白灯采用调色实测比例，其他颜色继续保持原来的全亮通道。 */
    switch (led_mode) {
    case SP01_LED_PRODUCT_CONTINUOUS:
    case SP01_LED_PRODUCT_CHARGING:
    case SP01_LED_PRODUCT_FULL:
    case SP01_LED_PRODUCT_LOW_WATER_WARNING_TYPEC:
        red_duty = 100U;
        green_duty = 60U;
        blue_duty = 85U;
        break;
    default: {
        sp01_indicator_pins_t pins;
        sp01_indicator_render(led_mode, buzzer_mode, &pins);
        red_duty = pins.red == APP_TRUE ? 100U : 0U;
        green_duty = pins.green == APP_TRUE ? 100U : 0U;
        blue_duty = pins.blue == APP_TRUE ? 100U : 0U;
        break;
    }
    }
#else
    switch (led_mode) {
    case SP01_LED_TEST_WHITE_BALANCE_1:
        /* 原始三路全开，作为对照。 */
        red_duty = 100U; green_duty = 100U; blue_duty = 100U;
        break;
    case SP01_LED_TEST_WHITE_BALANCE_2:
        /* 参考值：R=180/G=150/B=110（换算为约 70/60/43%）。 */
        red_duty = 70U; green_duty = 60U; blue_duty = 43U;
        break;
    case SP01_LED_TEST_WHITE_BALANCE_3:
        red_duty = 70U; green_duty = 60U; blue_duty = 50U;
        break;
    case SP01_LED_TEST_WHITE_BALANCE_4:
        red_duty = 70U; green_duty = 60U; blue_duty = 36U;
        break;
    case SP01_LED_TEST_WHITE_BALANCE_5:
        red_duty = 75U; green_duty = 55U; blue_duty = 43U;
        break;
    case SP01_LED_TEST_WHITE_BALANCE_6:
        red_duty = 65U; green_duty = 60U; blue_duty = 43U;
        break;
    default:
        break;
    }
#endif
    board_vendor_rgb_pwm_apply(red_duty, green_duty, blue_duty);
#if defined(SP01_SPEC_TEST007)
    {
        sp01_indicator_pins_t buzzer_pins;

        /* 007 调整 RGB 占空比时，蜂鸣器仍按业务状态和音调正常输出。 */
        sp01_indicator_render(led_mode, buzzer_mode, &buzzer_pins);
        board_vendor_buzzer_apply(buzzer_pins.buzzer_on,
                                  buzzer_frequency_hz,
                                  buzzer_duty_percent);
    }
#else
    /* 白平衡调色固件没有触摸业务，保持静音。 */
    board_vendor_buzzer_apply(APP_FALSE, 0U, 0U);
    (void)led_mode;
    (void)buzzer_mode;
    (void)buzzer_frequency_hz;
    (void)buzzer_duty_percent;
#endif
    return;
#else
#if defined(SP01_RGB_ISOLATED_PA5_TEST) || defined(SP01_RGB_ISOLATED_PB3_TEST)
    board_vendor_rgb_pa5_state_t state = BOARD_VENDOR_RGB_PA5_OFF;
    board_vendor_rgb_isolated_channel_t channel = BOARD_VENDOR_RGB_ISOLATED_PA5;

    if (led_mode == SP01_LED_TEST_ISOLATED_PA5_LOW ||
        led_mode == SP01_LED_TEST_ISOLATED_PB3_LOW) {
        state = BOARD_VENDOR_RGB_PA5_LOW;
    } else if (led_mode == SP01_LED_TEST_ISOLATED_PA5_HIGH ||
               led_mode == SP01_LED_TEST_ISOLATED_PB3_HIGH) {
        state = BOARD_VENDOR_RGB_PA5_HIGH;
    }
#ifdef SP01_RGB_ISOLATED_PB3_TEST
    channel = BOARD_VENDOR_RGB_ISOLATED_PB3;
#endif
    board_vendor_rgb_isolated_apply(channel, state);
    board_vendor_buzzer_apply(APP_FALSE, 0U, 0U);
    (void)buzzer_mode;
    (void)buzzer_frequency_hz;
    (void)buzzer_duty_percent;
    return;
#else
    sp01_indicator_pins_t pins;
#ifdef SP01_RGB_SINGLE_ACTIVE_HIGH
    const app_u8 rgb_active_level = BOARD_GPIO_ACTIVE_HIGH;
#else
    const app_u8 rgb_active_level = BOARD_RGB_ACTIVE_LEVEL;
#endif

    sp01_indicator_render(led_mode, buzzer_mode, &pins);

#ifdef SP01_RGB_TRISTATE
    /* RGB 隔离扫描：未选中的通道切高阻，避免主动拉高造成串电。 */
    board_vendor_rgb_tristate_apply(pins.red,
                                    pins.green,
                                    pins.blue,
                                    rgb_active_level);
    board_vendor_buzzer_apply(APP_FALSE, 0U, 0U);
    (void)buzzer_mode;
    (void)buzzer_frequency_hz;
    (void)buzzer_duty_percent;
#else
    board_vendor_gpio_write(BOARD_VENDOR_PIN_LED_RED,
                            board_output_level(pins.red, rgb_active_level));
    board_vendor_gpio_write(BOARD_VENDOR_PIN_LED_GREEN,
                            board_output_level(pins.green, rgb_active_level));
    board_vendor_gpio_write(BOARD_VENDOR_PIN_LED_BLUE,
                            board_output_level(pins.blue, rgb_active_level));
    board_vendor_buzzer_apply(pins.buzzer_on,
                              buzzer_frequency_hz,
                              buzzer_duty_percent);
#endif
#endif
#endif
}
#endif

void board_safe_outputs_off(void)
{
#ifdef HOST_SIM
    g_sim_outputs.pump_enable = APP_FALSE;
    g_sim_outputs.radar_power_enable = APP_FALSE;
    g_sim_outputs.led_mode = SP01_LED_OFF;
    g_sim_outputs.buzzer_mode = SP01_BUZZER_OFF;
    g_sim_outputs.buzzer_frequency_hz = 0U;
    g_sim_outputs.buzzer_duty_percent = 0U;
#else
#if defined(SP01_PUMP_TEST) || defined(SP01_BATTERY_PUMP_TEST)
    /* 泵验证版：即使调用安全输出函数，也保持 PB2 高电平。 */
    board_vendor_gpio_write(BOARD_VENDOR_PIN_PUMP_ON,
                            board_output_level(APP_TRUE, BOARD_PUMP_ON_ACTIVE_LEVEL));
                            #else
    board_vendor_gpio_write(BOARD_VENDOR_PIN_PUMP_ON,
                            board_output_level(APP_FALSE, BOARD_PUMP_ON_ACTIVE_LEVEL));
#endif
    board_vendor_gpio_write(BOARD_VENDOR_PIN_RADAR_POWER,
                            board_output_level(APP_FALSE, BOARD_GPIO_ACTIVE_HIGH));
    board_set_indicators(SP01_LED_OFF, SP01_BUZZER_OFF, 0U, 0U);
#endif
}

void board_apply_outputs(const sp01_outputs_t *outputs)
{
    if (outputs == 0) {
        board_safe_outputs_off();
        return;
    }

#ifdef HOST_SIM
    g_sim_outputs = *outputs;
    g_sim_outputs.pump_enable = board_effective_pump_enable(outputs->pump_enable);
#else
    board_vendor_gpio_write(BOARD_VENDOR_PIN_PUMP_ON,
                            board_output_level(board_effective_pump_enable(outputs->pump_enable),
                                               BOARD_PUMP_ON_ACTIVE_LEVEL));
    board_vendor_gpio_write(BOARD_VENDOR_PIN_RADAR_POWER,
                            board_output_level(outputs->radar_power_enable,
                                               BOARD_GPIO_ACTIVE_HIGH));
    board_set_indicators(outputs->led_mode,
                         outputs->buzzer_mode,
                         outputs->buzzer_frequency_hz,
                         outputs->buzzer_duty_percent);
#endif
}

board_inputs_t board_read_inputs(void)
{
    board_inputs_t inputs;

#ifdef HOST_SIM
    inputs = g_sim_inputs;
#else
    inputs.radar_active = board_vendor_gpio_read(BOARD_VENDOR_PIN_RADAR_INPUT) ==
                          (BOARD_RADAR_ACTIVE_LEVEL == BOARD_GPIO_ACTIVE_HIGH ? APP_TRUE : APP_FALSE) ?
                          APP_TRUE : APP_FALSE;
    inputs.key_pressed = board_vendor_gpio_read(BOARD_VENDOR_PIN_KEY) ==
                         (BOARD_KEY_ACTIVE_LEVEL == BOARD_GPIO_ACTIVE_HIGH ? APP_TRUE : APP_FALSE) ?
                         APP_TRUE : APP_FALSE;
    inputs.key_long_pressed = APP_FALSE;
    inputs.charging_active = board_vendor_gpio_read(BOARD_VENDOR_PIN_CHARGE_ACTIVE) ==
                             (BOARD_CHARGE_ACTIVE_LEVEL == BOARD_GPIO_ACTIVE_HIGH ? APP_TRUE : APP_FALSE) ?
                             APP_TRUE : APP_FALSE;
    inputs.charge_done = board_vendor_gpio_read(BOARD_VENDOR_PIN_CHARGE_DONE) ==
                         (BOARD_CHARGE_DONE_LEVEL == BOARD_GPIO_ACTIVE_HIGH ? APP_TRUE : APP_FALSE) ?
                         APP_TRUE : APP_FALSE;
    inputs.pump_sense_mv = board_vendor_adc_read_mv(BOARD_VENDOR_ADC_PUMP_SENSE);
    inputs.battery_mv = board_vendor_adc_read_mv(BOARD_VENDOR_ADC_BATTERY);
    inputs.ntc_mv = board_vendor_adc_read_mv(BOARD_VENDOR_ADC_NTC);
#endif

    return inputs;
}

app_u16 board_adc_read_channel_mv(app_u8 adc_channel)
{
#ifdef HOST_SIM
    (void)adc_channel;
    return 0U;
#else
    return board_vendor_adc_read_channel_mv(adc_channel);
#endif
}

app_u16 board_adc_read_channel_raw(app_u8 adc_channel)
{
#ifdef HOST_SIM
    (void)adc_channel;
    return 0U;
#else
    return board_vendor_adc_read_channel_raw(adc_channel);
#endif
}

app_bool_t board_take_10ms_tick(void)
{
#ifndef HOST_SIM
    if (board_vendor_take_10ms_tick() == APP_TRUE) {
        return APP_TRUE;
    }
#endif
    if (g_tick_10ms_ready != 0U) {
        g_tick_10ms_ready = 0U;
        return APP_TRUE;
    }

    return APP_FALSE;
}

void board_tick_10ms_hook(void)
{
    g_tick_10ms_ready = 1U;
}

#ifdef HOST_SIM
void board_sim_set_inputs(board_inputs_t inputs)
{
    g_sim_inputs = inputs;
}

sp01_outputs_t board_sim_get_outputs(void)
{
    return g_sim_outputs;
}

void board_sim_force_tick(void)
{
    board_tick_10ms_hook();
}
#endif
