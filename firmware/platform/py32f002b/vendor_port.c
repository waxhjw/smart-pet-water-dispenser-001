#include "board_port_vendor.h"
#include "../../app/board_profile.h"
#include "../../app/app_config.h"
#include "../../app/sp01_build_info.h"

#include "py32f0xx_hal.h"

static ADC_HandleTypeDef g_adc;
static TIM_HandleTypeDef g_buzzer_timer;
static IWDG_HandleTypeDef g_iwdg;
static app_bool_t g_buzzer_pwm_running;
static app_u16 g_buzzer_frequency_hz;
static app_u8 g_buzzer_duty_percent;
static app_u32 g_last_tick_ms;
static board_vendor_reset_reason_t g_reset_reason;
#if defined(SP01_RGB_WHITE_BALANCE_TEST) || defined(SP01_SPEC_TEST007)
static volatile app_u8 g_rgb_red_duty_percent;
static volatile app_u8 g_rgb_green_duty_percent;
static volatile app_u8 g_rgb_blue_duty_percent;
#endif
#ifdef SP01_RGB_WHITE_BALANCE_TEST
static volatile app_u8 g_rgb_pwm_phase_ms;
static volatile app_bool_t g_rgb_pwm_ready;
#endif
#ifdef SP01_SPEC_TEST007
static TIM_HandleTypeDef g_rgb_pwm_timer;
static volatile app_u8 g_rgb_pwm_phase;
static volatile app_bool_t g_rgb_pwm_ready;
#endif
#ifdef SP01_RGB_TUNER_TEST
/* 上位机经 DAPLink/SWD 写这三个对齐的32位RAM变量；MCU继续运行。 */
/* 放在 RAM 并由初始化代码读取，保证链接器保留这个可供上位机核验的版本标记。 */
volatile app_u32 g_sp01_rgb_tuner_profile_id = SP01_BUILD_PROFILE_RGB_TUNER_TEST;
volatile app_u32 g_sp01_rgb_tuner_red_duty_percent = 70UL;
volatile app_u32 g_sp01_rgb_tuner_green_duty_percent = 60UL;
volatile app_u32 g_sp01_rgb_tuner_blue_duty_percent = 30UL;
static TIM_HandleTypeDef g_rgb_tuner_timer;
static volatile app_u8 g_rgb_tuner_phase;
static volatile app_bool_t g_rgb_tuner_ready;
#endif

static void board_vendor_fail_safe_halt(void)
{
    /*
     * 初始化失败时不继续跑产品状态机。
     * 真实 GPIO 已先配置为安全电平，避免 ADC/时钟初始化失败后误开泵。
     */
    for (;;) {
    }
}

static GPIO_TypeDef *board_vendor_gpio_port(board_vendor_pin_t pin)
{
    switch (pin) {
    case BOARD_VENDOR_PIN_PUMP_ON:
    case BOARD_VENDOR_PIN_KEY:
    case BOARD_VENDOR_PIN_CHARGE_ACTIVE:
    case BOARD_VENDOR_PIN_CHARGE_DONE:
    case BOARD_VENDOR_PIN_LED_RED:
    case BOARD_VENDOR_PIN_BUZZER:
        return GPIOB;
    case BOARD_VENDOR_PIN_RADAR_POWER:
    case BOARD_VENDOR_PIN_LED_BLUE:
    case BOARD_VENDOR_PIN_LED_GREEN:
        return GPIOA;
    case BOARD_VENDOR_PIN_RADAR_INPUT:
        return GPIOC;
    default:
        return (GPIO_TypeDef *)0;
    }
}

static uint16_t board_vendor_gpio_pin(board_vendor_pin_t pin)
{
    switch (pin) {
    case BOARD_VENDOR_PIN_PUMP_ON:
        return GPIO_PIN_2;
    case BOARD_VENDOR_PIN_RADAR_POWER:
        return GPIO_PIN_7;
    case BOARD_VENDOR_PIN_RADAR_INPUT:
        return GPIO_PIN_1;
    case BOARD_VENDOR_PIN_KEY:
        return GPIO_PIN_0;
    case BOARD_VENDOR_PIN_CHARGE_ACTIVE:
        return GPIO_PIN_4;
    case BOARD_VENDOR_PIN_CHARGE_DONE:
        return GPIO_PIN_5;
    case BOARD_VENDOR_PIN_LED_RED:
        return GPIO_PIN_3;
    case BOARD_VENDOR_PIN_LED_GREEN:
        return GPIO_PIN_5;
    case BOARD_VENDOR_PIN_LED_BLUE:
        return GPIO_PIN_1;
    case BOARD_VENDOR_PIN_BUZZER:
        return GPIO_PIN_7;
    default:
        return 0U;
    }
}

static void board_vendor_config_gpio(void)
{
    GPIO_InitTypeDef output = {0};
    GPIO_InitTypeDef input = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* 先写输出锁存器，再切换为推挽输出，减少复位瞬间毛刺。 */
#if defined(SP01_PUMP_TEST) || defined(SP01_BATTERY_PUMP_TEST)
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET); /* 水泵验证版保持 Pump_ON 高电平 */
#else
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET); /* 其它版本上电先关闭水泵 */
#endif
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET); /* REL_EN 关闭 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);   /* RGB 红灭 */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);   /* RGB 绿灭 */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);   /* RGB 蓝灭 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET); /* 蜂鸣器关闭 */

    output.Mode = GPIO_MODE_OUTPUT_PP;
    output.Pull = GPIO_NOPULL;
    output.Speed = GPIO_SPEED_FREQ_LOW;

    output.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOB, &output);
    output.Pin = GPIO_PIN_1 | GPIO_PIN_5 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &output);

    input.Mode = GPIO_MODE_INPUT;
    input.Pull = GPIO_NOPULL;
    input.Speed = GPIO_SPEED_FREQ_LOW;

    input.Pin = GPIO_PIN_0; /* KEY */
    HAL_GPIO_Init(GPIOB, &input);
    input.Pin = GPIO_PIN_1; /* REL */
    HAL_GPIO_Init(GPIOC, &input);
    input.Pin = GPIO_PIN_4 | GPIO_PIN_5; /* CHRG/DONE，低有效 */
    input.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &input);
}

static void board_vendor_buzzer_gpio_low(void)
{
    GPIO_InitTypeDef output = {0};

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
    output.Pin = GPIO_PIN_7;
    output.Mode = GPIO_MODE_OUTPUT_PP;
    output.Pull = GPIO_NOPULL;
    output.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &output);
}

static void board_vendor_config_buzzer_channel(app_u8 duty_percent)
{
    TIM_OC_InitTypeDef channel = {0};
    uint32_t pulse;

    channel.OCMode = TIM_OCMODE_PWM1;
    pulse = ((g_buzzer_timer.Init.Period + 1U) *
             (uint32_t)duty_percent) / 100UL;
    if (pulse == 0UL) {
        pulse = 1UL;
    }
    channel.Pulse = pulse;
    channel.OCPolarity = TIM_OCPOLARITY_HIGH;
    channel.OCFastMode = TIM_OCFAST_DISABLE;
    channel.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    channel.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    channel.OCIdleState = TIM_OCIDLESTATE_RESET;

    if (HAL_TIM_PWM_ConfigChannel(&g_buzzer_timer,
                                  &channel,
                                  TIM_CHANNEL_1) != HAL_OK) {
        board_vendor_fail_safe_halt();
    }
}

static void board_vendor_buzzer_gpio_af(void)
{
    GPIO_InitTypeDef alternate = {0};

    alternate.Pin = GPIO_PIN_7;
    alternate.Mode = GPIO_MODE_AF_PP;
    alternate.Pull = GPIO_NOPULL;
    alternate.Speed = GPIO_SPEED_FREQ_LOW;
    alternate.Alternate = GPIO_AF5_TIM14;
    HAL_GPIO_Init(GPIOB, &alternate);
}

static void board_vendor_config_buzzer_pwm(void)
{
    g_buzzer_timer.Instance = TIM14;
    g_buzzer_timer.Init.Prescaler = 0U;
    g_buzzer_timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_buzzer_timer.Init.Period = (uint32_t)((HSI_VALUE / 2000UL) - 1UL);
    g_buzzer_timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    g_buzzer_timer.Init.RepetitionCounter = 0U;
    g_buzzer_timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    g_buzzer_frequency_hz = 2000U;
    g_buzzer_duty_percent = 50U;

    if (HAL_TIM_PWM_Init(&g_buzzer_timer) != HAL_OK) {
        board_vendor_fail_safe_halt();
    }

    board_vendor_config_buzzer_channel(g_buzzer_duty_percent);

    g_buzzer_pwm_running = APP_FALSE;
    board_vendor_buzzer_gpio_low();
}

static void board_vendor_config_adc(void)
{
    if (HAL_ADC_Init(&g_adc) != HAL_OK) {
        board_vendor_fail_safe_halt();
    }

    if (HAL_ADCEx_Calibration_Start(&g_adc) != HAL_OK) {
        board_vendor_fail_safe_halt();
    }
}

static void board_vendor_capture_reset_reason(void)
{
    g_reset_reason = BOARD_VENDOR_RESET_UNKNOWN;

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET) {
        g_reset_reason = BOARD_VENDOR_RESET_IWDG;
    }

    /* 读取后清除复位标志，避免后续普通上电一直被误认为看门狗复位。 */
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

static void board_vendor_config_watchdog(void)
{
    /*
     * LSI 约 32.768kHz，64 分频后约 512Hz；reload=1023 时约 2 秒超时。
     * 这个时间明显大于正常 10ms 主循环周期，也能覆盖一次 ADC/状态处理抖动。
     */
    g_iwdg.Instance = IWDG;
    g_iwdg.Init.Prescaler = IWDG_PRESCALER_64;
    g_iwdg.Init.Reload = 1023U;

    if (HAL_IWDG_Init(&g_iwdg) != HAL_OK) {
        board_vendor_fail_safe_halt();
    }
}

void board_vendor_init(void)
{
    g_adc.Instance = ADC1;

    board_vendor_capture_reset_reason();

    if (HAL_Init() != HAL_OK) {
        board_vendor_fail_safe_halt();
    }

    board_vendor_config_gpio();
#ifdef SP01_RGB_WHITE_BALANCE_TEST
    g_rgb_red_duty_percent = 0U;
    g_rgb_green_duty_percent = 0U;
    g_rgb_blue_duty_percent = 0U;
    g_rgb_pwm_phase_ms = 0U;
    g_rgb_pwm_ready = APP_TRUE;
#endif
#ifdef SP01_SPEC_TEST007
    g_rgb_red_duty_percent = 0U;
    g_rgb_green_duty_percent = 0U;
    g_rgb_blue_duty_percent = 0U;
    g_rgb_pwm_phase = 0U;
    g_rgb_pwm_ready = APP_TRUE;
    g_rgb_pwm_timer.Instance = TIM1;
    g_rgb_pwm_timer.Init.Prescaler = 0U;
    g_rgb_pwm_timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_rgb_pwm_timer.Init.Period = (uint32_t)(HSI_VALUE /
        SP01_RGB_TUNER_TIMER_TICK_HZ) - 1UL;
    g_rgb_pwm_timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    g_rgb_pwm_timer.Init.RepetitionCounter = 0U;
    g_rgb_pwm_timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&g_rgb_pwm_timer) != HAL_OK) {
        board_vendor_fail_safe_halt();
    }
    if (HAL_TIM_Base_Start_IT(&g_rgb_pwm_timer) != HAL_OK) {
        board_vendor_fail_safe_halt();
    }
#endif
#ifdef SP01_RGB_TUNER_TEST
    g_rgb_tuner_phase = 0U;
    g_rgb_tuner_ready = APP_FALSE;
    /* 标记不匹配时让三路灯保持关闭，不启动调色 PWM。 */
    if (g_sp01_rgb_tuner_profile_id == SP01_BUILD_PROFILE_RGB_TUNER_TEST) {
        g_rgb_tuner_ready = APP_TRUE;
        g_rgb_tuner_timer.Instance = TIM14;
        g_rgb_tuner_timer.Init.Prescaler = 0U;
        g_rgb_tuner_timer.Init.CounterMode = TIM_COUNTERMODE_UP;
        g_rgb_tuner_timer.Init.Period = (uint32_t)(HSI_VALUE /
            SP01_RGB_TUNER_TIMER_TICK_HZ) - 1UL;
        g_rgb_tuner_timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
        g_rgb_tuner_timer.Init.RepetitionCounter = 0U;
        g_rgb_tuner_timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
        if (HAL_TIM_Base_Init(&g_rgb_tuner_timer) != HAL_OK) {
            board_vendor_fail_safe_halt();
        }
        HAL_NVIC_SetPriority(TIM14_IRQn, 0U, 0U);
        HAL_NVIC_EnableIRQ(TIM14_IRQn);
        if (HAL_TIM_Base_Start_IT(&g_rgb_tuner_timer) != HAL_OK) {
            board_vendor_fail_safe_halt();
        }
    }
#endif
#ifdef SP01_RGB_TUNER_TEST
    /* TIM14 在调色版专供 PWM 更新中断，不配置 PB7 蜂鸣器输出。 */
#else
    board_vendor_config_buzzer_pwm();
#endif

    g_adc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    g_adc.Init.Resolution = ADC_RESOLUTION_12B;
    g_adc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    g_adc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
    g_adc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    g_adc.Init.LowPowerAutoWait = DISABLE;
    g_adc.Init.ContinuousConvMode = DISABLE;
    g_adc.Init.DiscontinuousConvMode = DISABLE;
    g_adc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    g_adc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    g_adc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    g_adc.Init.SamplingTimeCommon = ADC_SAMPLETIME_239CYCLES_5;
    board_vendor_config_adc();
    board_vendor_config_watchdog();

    g_last_tick_ms = HAL_GetTick();
}

void board_vendor_watchdog_refresh(void)
{
    (void)HAL_IWDG_Refresh(&g_iwdg);
}

board_vendor_reset_reason_t board_vendor_get_reset_reason(void)
{
    return g_reset_reason;
}

void board_vendor_gpio_write(board_vendor_pin_t pin, app_bool_t level)
{
    GPIO_TypeDef *port = board_vendor_gpio_port(pin);
    uint16_t gpio_pin = board_vendor_gpio_pin(pin);

    if (port == (GPIO_TypeDef *)0 || gpio_pin == 0U) {
        return;
    }

    HAL_GPIO_WritePin(port,
                      gpio_pin,
                      level == APP_TRUE ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void board_vendor_rgb_pwm_apply(app_u8 red_duty_percent,
                                app_u8 green_duty_percent,
                                app_u8 blue_duty_percent)
{
#if defined(SP01_RGB_WHITE_BALANCE_TEST) || defined(SP01_SPEC_TEST007)
    g_rgb_red_duty_percent = red_duty_percent > 100U ? 100U : red_duty_percent;
    g_rgb_green_duty_percent = green_duty_percent > 100U ? 100U : green_duty_percent;
    g_rgb_blue_duty_percent = blue_duty_percent > 100U ? 100U : blue_duty_percent;
#else
    (void)red_duty_percent;
    (void)green_duty_percent;
    (void)blue_duty_percent;
#endif
}

void board_vendor_rgb_pwm_tick_1ms(void)
{
#ifdef SP01_RGB_WHITE_BALANCE_TEST
    app_u8 phase;
    app_u8 threshold;

    if (g_rgb_pwm_ready != APP_TRUE) {
        return;
    }

    g_rgb_pwm_phase_ms++;
    if (g_rgb_pwm_phase_ms >= SP01_RGB_WHITE_BALANCE_PWM_PERIOD_MS) {
        g_rgb_pwm_phase_ms = 0U;
    }

    phase = g_rgb_pwm_phase_ms;
    threshold = (app_u8)(phase * (100U / SP01_RGB_WHITE_BALANCE_PWM_PERIOD_MS));

    /* 板上 RGB 已按低电平点亮验证：有效时拉低，关闭时拉高。 */
    HAL_GPIO_WritePin(GPIOB,
                      GPIO_PIN_3,
                      g_rgb_red_duty_percent > threshold ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA,
                      GPIO_PIN_5,
                      g_rgb_green_duty_percent > threshold ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA,
                      GPIO_PIN_1,
                      g_rgb_blue_duty_percent > threshold ? GPIO_PIN_RESET : GPIO_PIN_SET);
#endif
}

#ifdef SP01_SPEC_TEST007
void board_vendor_rgb_pwm_tim1_irq(void)
{
    app_u8 phase;

    /* 先清中断标志；TIM1 每100微秒更新一次，100步构成10毫秒PWM周期。 */
    __HAL_TIM_CLEAR_FLAG(&g_rgb_pwm_timer, TIM_FLAG_UPDATE);
    if (g_rgb_pwm_ready != APP_TRUE) {
        return;
    }

    g_rgb_pwm_phase++;
    if (g_rgb_pwm_phase >= SP01_RGB_TUNER_PWM_STEP_COUNT) {
        g_rgb_pwm_phase = 0U;
    }
    phase = g_rgb_pwm_phase;

    /* 低电平点亮，100个时间片对应100%，支持1%分辨率且保留TIM14蜂鸣器。 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,
                      phase < g_rgb_red_duty_percent ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,
                      phase < g_rgb_green_duty_percent ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1,
                      phase < g_rgb_blue_duty_percent ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
#endif /* SP01_SPEC_TEST007 */

#ifdef SP01_RGB_TUNER_TEST
static void board_vendor_rgb_tuner_update(void)
{
    app_u8 phase;
    app_u32 red;
    app_u32 green;
    app_u32 blue;

    if (g_rgb_tuner_ready != APP_TRUE) {
        return;
    }

    g_rgb_tuner_phase++;
    if (g_rgb_tuner_phase >= SP01_RGB_TUNER_PWM_STEP_COUNT) {
        g_rgb_tuner_phase = 0U;
    }
    phase = g_rgb_tuner_phase;

    red = g_sp01_rgb_tuner_red_duty_percent;
    green = g_sp01_rgb_tuner_green_duty_percent;
    blue = g_sp01_rgb_tuner_blue_duty_percent;
    if (red > 100UL) red = 100UL;
    if (green > 100UL) green = 100UL;
    if (blue > 100UL) blue = 100UL;

    /* 共阳 RGB 低有效：100个10kHz时间片组成100Hz PWM，每格占空比1%。 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,
                      (app_u32)phase < red ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,
                      (app_u32)phase < green ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1,
                      (app_u32)phase < blue ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void board_vendor_rgb_tuner_tim14_irq(void)
{
    /* 直接清更新中断标志，避免为简单定时器中断引入整套高级定时器回调。 */
    __HAL_TIM_CLEAR_FLAG(&g_rgb_tuner_timer, TIM_FLAG_UPDATE);
    board_vendor_rgb_tuner_update();
}
#endif /* SP01_RGB_TUNER_TEST */

void board_vendor_rgb_tristate_apply(app_bool_t red,
                                     app_bool_t green,
                                     app_bool_t blue,
                                     app_u8 active_level)
{
    GPIO_InitTypeDef input = {0};
    GPIO_InitTypeDef output = {0};

    input.Mode = GPIO_MODE_INPUT;
    input.Pull = GPIO_NOPULL;
    input.Speed = GPIO_SPEED_FREQ_LOW;

    output.Mode = GPIO_MODE_OUTPUT_PP;
    output.Pull = GPIO_NOPULL;
    output.Speed = GPIO_SPEED_FREQ_LOW;

    if (red == APP_TRUE) {
        HAL_GPIO_WritePin(GPIOB,
                          GPIO_PIN_3,
                          active_level == BOARD_GPIO_ACTIVE_HIGH ? GPIO_PIN_SET : GPIO_PIN_RESET);
        output.Pin = GPIO_PIN_3;
        HAL_GPIO_Init(GPIOB, &output);
    } else {
        input.Pin = GPIO_PIN_3;
        HAL_GPIO_Init(GPIOB, &input);
    }

    if (green == APP_TRUE) {
        HAL_GPIO_WritePin(GPIOA,
                          GPIO_PIN_5,
                          active_level == BOARD_GPIO_ACTIVE_HIGH ? GPIO_PIN_SET : GPIO_PIN_RESET);
        output.Pin = GPIO_PIN_5;
        HAL_GPIO_Init(GPIOA, &output);
    } else {
        input.Pin = GPIO_PIN_5;
        HAL_GPIO_Init(GPIOA, &input);
    }

    if (blue == APP_TRUE) {
        HAL_GPIO_WritePin(GPIOA,
                          GPIO_PIN_1,
                          active_level == BOARD_GPIO_ACTIVE_HIGH ? GPIO_PIN_SET : GPIO_PIN_RESET);
        output.Pin = GPIO_PIN_1;
        HAL_GPIO_Init(GPIOA, &output);
    } else {
        input.Pin = GPIO_PIN_1;
        HAL_GPIO_Init(GPIOA, &input);
    }
}

void board_vendor_rgb_isolated_apply(board_vendor_rgb_isolated_channel_t channel,
                                     board_vendor_rgb_pa5_state_t state)
{
#if defined(SP01_RGB_ISOLATED_PA5_TEST) || defined(SP01_RGB_ISOLATED_PB3_TEST)
    GPIO_InitTypeDef input = {0};
    GPIO_InitTypeDef output = {0};

    /* PA1/PB3 不参与回路，避免 PA1 的红灯在 PA5 测试时串入。 */
    input.Mode = GPIO_MODE_INPUT;
    input.Pull = GPIO_NOPULL;
    input.Speed = GPIO_SPEED_FREQ_LOW;
    input.Pin = GPIO_PIN_1 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOA, &input);
    input.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOB, &input);

    if (state == BOARD_VENDOR_RGB_PA5_OFF) {
        return;
    }

    if (channel == BOARD_VENDOR_RGB_ISOLATED_PB3) {
        HAL_GPIO_WritePin(GPIOB,
                          GPIO_PIN_3,
                          state == BOARD_VENDOR_RGB_PA5_HIGH ? GPIO_PIN_SET : GPIO_PIN_RESET);
        output.Pin = GPIO_PIN_3;
        output.Mode = GPIO_MODE_OUTPUT_PP;
        output.Pull = GPIO_NOPULL;
        output.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOB, &output);
        return;
    }

    HAL_GPIO_WritePin(GPIOA,
                      GPIO_PIN_5,
                      state == BOARD_VENDOR_RGB_PA5_HIGH ? GPIO_PIN_SET : GPIO_PIN_RESET);
    output.Pin = GPIO_PIN_5;
    output.Mode = GPIO_MODE_OUTPUT_PP;
    output.Pull = GPIO_NOPULL;
    output.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &output);
#else
    (void)state;
#endif
}

void board_vendor_buzzer_apply(app_bool_t enabled,
                               app_u16 frequency_hz,
                               app_u8 duty_percent)
{
    if (enabled == APP_TRUE) {
        if (frequency_hz == 0U) {
            frequency_hz = 2000U;
        }
        if (duty_percent == 0U || duty_percent > 100U) {
            duty_percent = SP01_BUZZER_DEFAULT_DUTY_PERCENT;
        }

        if (g_buzzer_pwm_running == APP_TRUE &&
            (g_buzzer_frequency_hz != frequency_hz ||
             g_buzzer_duty_percent != duty_percent)) {
            (void)HAL_TIM_PWM_Stop(&g_buzzer_timer, TIM_CHANNEL_1);
            g_buzzer_pwm_running = APP_FALSE;
            board_vendor_buzzer_gpio_low();
        }

        if (g_buzzer_frequency_hz != frequency_hz ||
            g_buzzer_duty_percent != duty_percent) {
            g_buzzer_timer.Init.Period =
                (uint32_t)((HSI_VALUE / (uint32_t)frequency_hz) - 1UL);
            if (HAL_TIM_PWM_Init(&g_buzzer_timer) != HAL_OK) {
                board_vendor_fail_safe_halt();
            }
            g_buzzer_frequency_hz = frequency_hz;
            g_buzzer_duty_percent = duty_percent;
            board_vendor_config_buzzer_channel(g_buzzer_duty_percent);
        }

        if (g_buzzer_pwm_running == APP_FALSE) {
            board_vendor_buzzer_gpio_af();
            if (HAL_TIM_PWM_Start(&g_buzzer_timer, TIM_CHANNEL_1) != HAL_OK) {
                board_vendor_fail_safe_halt();
            }
            g_buzzer_pwm_running = APP_TRUE;
        }
    } else if (g_buzzer_pwm_running == APP_TRUE) {
        (void)HAL_TIM_PWM_Stop(&g_buzzer_timer, TIM_CHANNEL_1);
        g_buzzer_pwm_running = APP_FALSE;
        board_vendor_buzzer_gpio_low();
    }
}

app_bool_t board_vendor_gpio_read(board_vendor_pin_t pin)
{
    GPIO_TypeDef *port = board_vendor_gpio_port(pin);
    uint16_t gpio_pin = board_vendor_gpio_pin(pin);

    if (port == (GPIO_TypeDef *)0 || gpio_pin == 0U) {
        return APP_FALSE;
    }

    return HAL_GPIO_ReadPin(port, gpio_pin) == GPIO_PIN_SET ? APP_TRUE : APP_FALSE;
}

static app_bool_t board_vendor_adc_select_channel(uint32_t channel)
{
    ADC_ChannelConfTypeDef config = {0};
    app_u32 old_channel;

    /*
     * PY32 ADC 的 CHSELR 是累加选择寄存器。若只反复调用
     * HAL_ADC_ConfigChannel(RANK_CHANNEL_NUMBER)，旧通道不会自动清除，
     * 后续读到的是多通道序列的结果。每次单通道采样前先逐个清空，
     * 再按官方示例重新选择目标通道。
     */
    if (HAL_ADC_Stop(&g_adc) != HAL_OK) {
        return APP_FALSE;
    }

    for (old_channel = 0U; old_channel <= 7U; old_channel++) {
        config.Channel = old_channel;
        config.Rank = ADC_RANK_NONE;
        config.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
        if (HAL_ADC_ConfigChannel(&g_adc, &config) != HAL_OK) {
            return APP_FALSE;
        }
    }

    config.Channel = channel;
    config.Rank = ADC_RANK_CHANNEL_NUMBER;
    config.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;

    if (HAL_ADC_ConfigChannel(&g_adc, &config) != HAL_OK) {
        return APP_FALSE;
    }

    return APP_TRUE;
}

static app_u16 board_vendor_adc_raw(uint32_t channel)
{
    if (channel > 7U || board_vendor_adc_select_channel(channel) == APP_FALSE) {
        return 0U;
    }

    if (HAL_ADC_Start(&g_adc) != HAL_OK) {
        return 0U;
    }
    if (HAL_ADC_PollForConversion(&g_adc, 2U) != HAL_OK) {
        (void)HAL_ADC_Stop(&g_adc);
        return 0U;
    }

    {
        app_u16 sample = (app_u16)HAL_ADC_GetValue(&g_adc);
        (void)HAL_ADC_Stop(&g_adc);
        return sample;
    }
}

static app_u16 board_vendor_adc_to_mv(app_u16 raw)
{
    return (app_u16)(((app_u32)raw * 3300UL + 2047UL) / 4095UL);
}

app_u16 board_vendor_adc_read_mv(board_vendor_adc_t channel)
{
    app_u16 mv;

    switch (channel) {
    case BOARD_VENDOR_ADC_PUMP_SENSE:
        mv = board_vendor_adc_to_mv(board_vendor_adc_raw(ADC_CHANNEL_1));
        return mv;
    case BOARD_VENDOR_ADC_BATTERY:
        mv = board_vendor_adc_to_mv(board_vendor_adc_raw(ADC_CHANNEL_0));
        /* 按原理图的 100k/100k 分压还原电池端电压，即采样点电压乘 2。 */
        return (app_u16)(((app_u32)mv * BOARD_VBAT_DIVIDER_GAIN_NUM +
                          (BOARD_VBAT_DIVIDER_GAIN_DEN / 2UL)) /
                         BOARD_VBAT_DIVIDER_GAIN_DEN);
    case BOARD_VENDOR_ADC_NTC:
        return board_vendor_adc_to_mv(board_vendor_adc_raw(ADC_CHANNEL_3));
    default:
        return 0U;
    }
}

app_u16 board_vendor_adc_read_channel_raw(app_u8 adc_channel)
{
    if (adc_channel > 7U) {
        return 0U;
    }

    return board_vendor_adc_raw((uint32_t)adc_channel);
}

app_u16 board_vendor_adc_read_channel_mv(app_u8 adc_channel)
{
    return board_vendor_adc_to_mv(board_vendor_adc_read_channel_raw(adc_channel));
}

app_bool_t board_vendor_take_10ms_tick(void)
{
    app_u32 now = HAL_GetTick();

    if ((now - g_last_tick_ms) >= 10UL) {
        g_last_tick_ms += 10UL;
        return APP_TRUE;
    }

    return APP_FALSE;
}
