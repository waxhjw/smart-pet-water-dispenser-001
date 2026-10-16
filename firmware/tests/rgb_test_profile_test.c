#include <assert.h>

#include "app/app_config.h"
#include "app/sp01_rgb_test.h"

static void assert_safe(sp01_outputs_t output, sp01_led_mode_t led_mode)
{
    assert(output.pump_enable == APP_FALSE);
    assert(output.radar_power_enable == APP_FALSE);
    assert(output.led_mode == led_mode);
    assert(output.buzzer_mode == SP01_BUZZER_OFF);
    assert(output.buzzer_frequency_hz == 0U);
}

int main(void)
{
    sp01_rgb_test_t test;
    sp01_outputs_t output;
    app_u32 index;

    sp01_rgb_test_init(&test);

    /* PB3 通道亮 2 秒，之后熄灭 1 秒。 */
    output = sp01_rgb_test_tick(&test);
    assert_safe(output, SP01_LED_TEST_PB3);
    for (index = 1UL; index < (SP01_RGB_TEST_CHANNEL_ON_MS / SP01_TICK_MS); index++) {
        output = sp01_rgb_test_tick(&test);
        assert_safe(output, SP01_LED_TEST_PB3);
    }
    output = sp01_rgb_test_tick(&test);
    assert_safe(output, SP01_LED_OFF);

    /* 熄灭结束后切换到 PA5，且泵、雷达和蜂鸣器仍保持关闭。 */
    for (index = 1UL; index < (SP01_RGB_TEST_CHANNEL_OFF_MS / SP01_TICK_MS); index++) {
        output = sp01_rgb_test_tick(&test);
        assert_safe(output, SP01_LED_OFF);
    }
    output = sp01_rgb_test_tick(&test);
    assert_safe(output, SP01_LED_TEST_PA5);

    /* 再完成 PA5 的亮灭后应切换到 PA1。 */
    for (index = 1UL; index < (SP01_RGB_TEST_CHANNEL_ON_MS / SP01_TICK_MS); index++) {
        output = sp01_rgb_test_tick(&test);
        assert_safe(output, SP01_LED_TEST_PA5);
    }
    output = sp01_rgb_test_tick(&test);
    assert_safe(output, SP01_LED_OFF);
    for (index = 1UL; index < (SP01_RGB_TEST_CHANNEL_OFF_MS / SP01_TICK_MS); index++) {
        output = sp01_rgb_test_tick(&test);
        assert_safe(output, SP01_LED_OFF);
    }
    output = sp01_rgb_test_tick(&test);
    assert_safe(output, SP01_LED_TEST_PA1);

    return 0;
}
