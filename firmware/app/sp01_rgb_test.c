#include "sp01_rgb_test.h"
#include "app_config.h"

#if defined(SP01_RGB_WHITE_BALANCE_TEST)
#define SP01_RGB_TEST_PHASE_COUNT  12U
#elif defined(SP01_RGB_COMBO_SCAN_TEST)
#define SP01_RGB_TEST_PHASE_COUNT  8U
#elif defined(SP01_RGB_COMBO_PA5_PA1_TEST)
#define SP01_RGB_TEST_PHASE_COUNT  2U
#elif defined(SP01_RGB_ISOLATED_PA5_TEST) || defined(SP01_RGB_ISOLATED_PB3_TEST)
#define SP01_RGB_TEST_PHASE_COUNT  4U
#else
#define SP01_RGB_TEST_PHASE_COUNT  6U
#endif

void sp01_rgb_test_init(sp01_rgb_test_t *test)
{
    if (test == 0) {
        return;
    }

    test->phase = 0U;
    test->phase_elapsed_ms = 0UL;
}

sp01_outputs_t sp01_rgb_test_tick(sp01_rgb_test_t *test)
{
#if !defined(SP01_RGB_TUNER_TEST) && !defined(SP01_RGB_SINGLE_TEST) && !defined(SP01_RGB_ALL_ON_TEST) && \
    !defined(SP01_RGB_WHITE_BALANCE_TEST) && \
    !defined(SP01_RGB_ISOLATED_PA5_TEST) && \
    !defined(SP01_RGB_ISOLATED_PB3_TEST) && !defined(SP01_RGB_COMBO_PA5_PA1_TEST) && \
    !defined(SP01_RGB_COMBO_SCAN_TEST)
    static const sp01_led_mode_t channel_modes[3] = {
        SP01_LED_TEST_PB3,
        SP01_LED_TEST_PA5,
        SP01_LED_TEST_PA1
    };
#endif
    sp01_outputs_t outputs;
#if !defined(SP01_RGB_TUNER_TEST) && !defined(SP01_RGB_SINGLE_TEST) && !defined(SP01_RGB_ALL_ON_TEST)
    app_u32 phase_duration_ms;
#endif

    outputs.pump_enable = APP_FALSE;
    outputs.radar_power_enable = APP_FALSE;
    outputs.led_mode = SP01_LED_OFF;
    outputs.buzzer_mode = SP01_BUZZER_OFF;
    outputs.buzzer_frequency_hz = 0U;
    outputs.buzzer_duty_percent = 0U;

    if (test == 0) {
        return outputs;
    }

#ifdef SP01_RGB_TUNER_TEST
    /* 上位机调色时由硬件层PWM驱动，所有业务执行器保持关闭。 */
    return outputs;
#elif defined(SP01_RGB_WHITE_BALANCE_TEST)
    /*
     * 先显示原始全开，再显示 R=180/G=150/B=110 的换算值及其附近档位。
     * 每个亮灯阶段持续 5 秒，随后熄灭 1 秒；全序列循环播放。
     */
    if ((test->phase & 1U) == 0U) {
        app_u8 step = (app_u8)(test->phase / 2U);
        outputs.led_mode = (sp01_led_mode_t)(SP01_LED_TEST_WHITE_BALANCE_1 + step);
        phase_duration_ms = SP01_RGB_WHITE_BALANCE_ON_MS;
    } else {
        outputs.led_mode = SP01_LED_OFF;
        phase_duration_ms = SP01_RGB_WHITE_BALANCE_OFF_MS;
    }

    test->phase_elapsed_ms += SP01_TICK_MS;
    if (test->phase_elapsed_ms >= phase_duration_ms) {
        test->phase_elapsed_ms = 0UL;
        test->phase++;
        if (test->phase >= SP01_RGB_TEST_PHASE_COUNT) {
            test->phase = 0U;
        }
    }
    return outputs;
#elif defined(SP01_RGB_SINGLE_TEST)
    /* 单通道诊断版：只保持指定 MCU 通道点亮，便于无万用表时肉眼确认。 */
#if defined(SP01_RGB_CHANNEL_PB3)
    outputs.led_mode = SP01_LED_TEST_PB3;
#elif defined(SP01_RGB_CHANNEL_PA5)
    outputs.led_mode = SP01_LED_TEST_PA5;
#elif defined(SP01_RGB_CHANNEL_PA1)
    outputs.led_mode = SP01_LED_TEST_PA1;
#else
#error "SP01_RGB_SINGLE_TEST requires SP01_RGB_CHANNEL_PB3, PA5, or PA1."
#endif
    return outputs;
#elif defined(SP01_RGB_ALL_ON_TEST)
    /* 三路同时常亮，单独确认实物 RGB 混色；水泵、雷达和蜂鸣器保持关闭。 */
    outputs.led_mode = SP01_LED_TEST_COMBO_ALL;
    return outputs;
#elif defined(SP01_RGB_COMBO_PA5_PA1_TEST)
    /* 复现旧版触摸出水：PA5 与 PA1 同时点亮，PB3 保持熄灭。 */
    if (test->phase == 0U) {
        outputs.led_mode = SP01_LED_TEST_COMBO_PA5_PA1;
        phase_duration_ms = SP01_RGB_TEST_CHANNEL_ON_MS;
    } else {
        phase_duration_ms = SP01_RGB_TEST_CHANNEL_OFF_MS;
    }

    test->phase_elapsed_ms += SP01_TICK_MS;
    if (test->phase_elapsed_ms >= phase_duration_ms) {
        test->phase_elapsed_ms = 0UL;
        test->phase++;
        if (test->phase >= SP01_RGB_TEST_PHASE_COUNT) {
            test->phase = 0U;
        }
    }
    return outputs;
#elif defined(SP01_RGB_COMBO_SCAN_TEST)
    /* 共阳 RGB 的 8 种物理组合：依次扫描红、绿、红绿、蓝、红蓝、绿蓝、全亮。 */
    switch (test->phase) {
    case 0U:
        outputs.led_mode = SP01_LED_OFF;
        break;
    case 1U:
        outputs.led_mode = SP01_LED_TEST_PB3;
        break;
    case 2U:
        outputs.led_mode = SP01_LED_TEST_PA5;
        break;
    case 3U:
        outputs.led_mode = SP01_LED_TEST_COMBO_PB3_PA5;
        break;
    case 4U:
        outputs.led_mode = SP01_LED_TEST_PA1;
        break;
    case 5U:
        outputs.led_mode = SP01_LED_TEST_COMBO_PB3_PA1;
        break;
    case 6U:
        outputs.led_mode = SP01_LED_TEST_COMBO_PA5_PA1;
        break;
    default:
        outputs.led_mode = SP01_LED_TEST_COMBO_ALL;
        break;
    }

    phase_duration_ms = SP01_RGB_COMBO_SCAN_STEP_MS;
    test->phase_elapsed_ms += SP01_TICK_MS;
    if (test->phase_elapsed_ms >= phase_duration_ms) {
        test->phase_elapsed_ms = 0UL;
        test->phase++;
        if (test->phase >= SP01_RGB_TEST_PHASE_COUNT) {
            test->phase = 0U;
        }
    }
    return outputs;
#elif defined(SP01_RGB_ISOLATED_PA5_TEST) || defined(SP01_RGB_ISOLATED_PB3_TEST)
    /*
     * 隔离版只观察 PA5：board_port 会把 PA1/PB3 切成高阻输入，
     * 这里分别输出低、高两个窗口，用于区分共阳/共阴和实际通道连接。
     */
    if (test->phase == 0U) {
#if defined(SP01_RGB_ISOLATED_PA5_TEST)
        outputs.led_mode = SP01_LED_TEST_ISOLATED_PA5_LOW;
#else
        outputs.led_mode = SP01_LED_TEST_ISOLATED_PB3_LOW;
#endif
        phase_duration_ms = SP01_RGB_ISOLATED_PA5_LOW_MS;
    } else if (test->phase == 1U) {
        phase_duration_ms = SP01_RGB_TEST_CHANNEL_OFF_MS;
    } else if (test->phase == 2U) {
#if defined(SP01_RGB_ISOLATED_PA5_TEST)
        outputs.led_mode = SP01_LED_TEST_ISOLATED_PA5_HIGH;
#else
        outputs.led_mode = SP01_LED_TEST_ISOLATED_PB3_HIGH;
#endif
        phase_duration_ms = SP01_RGB_ISOLATED_PA5_HIGH_MS;
    } else {
        phase_duration_ms = SP01_RGB_TEST_CHANNEL_OFF_MS;
    }

    test->phase_elapsed_ms += SP01_TICK_MS;
    if (test->phase_elapsed_ms >= phase_duration_ms) {
        test->phase_elapsed_ms = 0UL;
        test->phase++;
        if (test->phase >= SP01_RGB_TEST_PHASE_COUNT) {
            test->phase = 0U;
        }
    }
    return outputs;
#else
    if ((test->phase & 1U) == 0U) {
        outputs.led_mode = channel_modes[test->phase / 2U];
        phase_duration_ms = SP01_RGB_TEST_CHANNEL_ON_MS;
    } else {
        phase_duration_ms = SP01_RGB_TEST_CHANNEL_OFF_MS;
    }

    test->phase_elapsed_ms += SP01_TICK_MS;
    if (test->phase_elapsed_ms >= phase_duration_ms) {
        test->phase_elapsed_ms = 0UL;
        test->phase++;
        if (test->phase >= SP01_RGB_TEST_PHASE_COUNT) {
            test->phase = 0U;
        }
    }

    return outputs;
#endif
}
