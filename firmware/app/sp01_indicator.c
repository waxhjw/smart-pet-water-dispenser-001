#include "sp01_indicator.h"

void sp01_indicator_render(sp01_led_mode_t led_mode,
                           sp01_buzzer_mode_t buzzer_mode,
                           sp01_indicator_pins_t *pins)
{
    if (pins == 0) {
        return;
    }

    pins->red = APP_FALSE;
    pins->green = APP_FALSE;
    pins->blue = APP_FALSE;
    pins->buzzer_on = buzzer_mode == SP01_BUZZER_OFF ? APP_FALSE : APP_TRUE;

    switch (led_mode) {
    case SP01_LED_CHARGING:
        pins->blue = APP_TRUE;
        break;
    case SP01_LED_FULL:
        pins->green = APP_TRUE;
        break;
    case SP01_LED_LOW_WATER:
    case SP01_LED_FAULT:
        pins->red = APP_TRUE;
        break;
    case SP01_LED_PUMPING:
        pins->green = APP_TRUE;
        pins->blue = APP_TRUE;
        break;
    case SP01_LED_SENSOR_READY:
        pins->green = APP_TRUE;
        break;
    case SP01_LED_SENSOR_PUMPING:
        pins->green = APP_TRUE;
        break;
    case SP01_LED_CONTINUOUS:
        /* 整机测试连续出水：固定蓝灯，避免和普通出水混淆。 */
        pins->blue = APP_TRUE;
        break;
    case SP01_LED_TEST_PB3:
        pins->blue = APP_TRUE;
        break;
    case SP01_LED_TEST_PA5:
        pins->green = APP_TRUE;
        break;
    case SP01_LED_TEST_PA1:
        pins->red = APP_TRUE;
        break;
    case SP01_LED_TEST_ISOLATED_PA5_LOW:
    case SP01_LED_TEST_ISOLATED_PA5_HIGH:
    case SP01_LED_TEST_ISOLATED_PB3_LOW:
    case SP01_LED_TEST_ISOLATED_PB3_HIGH:
        /* 目标板隔离测试由 board_port 直接配置 GPIO，高层不点亮其它通道。 */
        break;
    case SP01_LED_TEST_COMBO_PA5_PA1:
        pins->red = APP_TRUE;
        pins->green = APP_TRUE;
        break;
    case SP01_LED_TEST_COMBO_PB3_PA5:
        pins->green = APP_TRUE;
        pins->blue = APP_TRUE;
        break;
    case SP01_LED_TEST_COMBO_PB3_PA1:
        pins->blue = APP_TRUE;
        pins->red = APP_TRUE;
        break;
    case SP01_LED_TEST_COMBO_ALL:
    case SP01_LED_TEST_WHITE_BALANCE_1:
    case SP01_LED_TEST_WHITE_BALANCE_2:
    case SP01_LED_TEST_WHITE_BALANCE_3:
    case SP01_LED_TEST_WHITE_BALANCE_4:
    case SP01_LED_TEST_WHITE_BALANCE_5:
    case SP01_LED_TEST_WHITE_BALANCE_6:
        pins->red = APP_TRUE;
        pins->green = APP_TRUE;
        pins->blue = APP_TRUE;
        break;
    case SP01_LED_PRODUCT_CONTINUOUS:
    case SP01_LED_PRODUCT_CHARGING:
    case SP01_LED_PRODUCT_FULL:
        pins->red = APP_TRUE;
        pins->green = APP_TRUE;
        pins->blue = APP_TRUE;
        break;
    case SP01_LED_PRODUCT_LOW_BATTERY:
        pins->red = APP_TRUE;
        break;
    case SP01_LED_PRODUCT_LOW_WATER_WARNING_TYPEC:
        /* Type-C 低水位预警使用白灯，与电池模式区分。 */
        pins->red = APP_TRUE;
        pins->green = APP_TRUE;
        pins->blue = APP_TRUE;
        break;
    case SP01_LED_PRODUCT_LOW_WATER_WARNING_BATTERY:
        /* 电池低水位预警使用绿灯。 */
        pins->green = APP_TRUE;
        break;
    case SP01_LED_OFF:
    case SP01_LED_STANDBY:
    default:
        break;
    }
}
