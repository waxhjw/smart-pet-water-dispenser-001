#include "sp01_pump_adc_test.h"
#include "app_config.h"

static sp01_outputs_t sp01_pump_adc_test_outputs(app_bool_t pump_enable,
                                                 sp01_buzzer_mode_t buzzer_mode)
{
    sp01_outputs_t outputs;

    outputs.pump_enable = pump_enable;
    outputs.radar_power_enable = APP_FALSE;
    outputs.led_mode = SP01_LED_OFF;
    outputs.buzzer_mode = buzzer_mode;
    outputs.buzzer_frequency_hz = 0U;
    outputs.buzzer_duty_percent = 0U;
    return outputs;
}

static sp01_buzzer_mode_t sp01_pump_adc_test_report_buzzer(sp01_pump_diag_t diag)
{
    switch (diag) {
    case SP01_PUMP_DIAG_NORMAL:
        /* 一声高音：采样落在正常区间。 */
        return SP01_BUZZER_RADAR_TEST;
    case SP01_PUMP_DIAG_LOW_WATER_SUSPECT:
        /* 一声低音：落在低水位疑似区间。 */
        return SP01_BUZZER_LOW_WATER;
    case SP01_PUMP_DIAG_NO_LOAD:
        /* 一声更低的错误音：接近无负载/采样过低。 */
        return SP01_BUZZER_ERROR;
    case SP01_PUMP_DIAG_OVER_CURRENT:
        /* 持续周期音：采样超过过流阈值。 */
        return SP01_BUZZER_FAULT;
    case SP01_PUMP_DIAG_UNKNOWN:
    default:
        /* 两声：窗口内没有得到有效分类。 */
        return SP01_BUZZER_DOUBLE_CLICK;
    }
}

void sp01_pump_adc_test_init(sp01_pump_adc_test_t *test)
{
    if (test == 0) {
        return;
    }

    test->state = SP01_PUMP_ADC_TEST_IDLE;
    test->key_was_pressed = APP_FALSE;
    test->report_beep_pending = APP_FALSE;
    test->elapsed_ms = 0UL;
    test->report_diag = SP01_PUMP_DIAG_UNKNOWN;
}

sp01_outputs_t sp01_pump_adc_test_update(sp01_pump_adc_test_t *test,
                                         app_bool_t key_pressed,
                                         sp01_pump_diag_t pump_diag)
{
    sp01_buzzer_mode_t buzzer_mode;

    if (test == 0) {
        return sp01_pump_adc_test_outputs(APP_FALSE, SP01_BUZZER_OFF);
    }

    buzzer_mode = SP01_BUZZER_OFF;

    switch (test->state) {
    case SP01_PUMP_ADC_TEST_IDLE:
        if (key_pressed == APP_TRUE && test->key_was_pressed == APP_FALSE) {
            /* 只有短按上升沿启动一轮，避免上电自动开泵。 */
            test->state = SP01_PUMP_ADC_TEST_RUNNING;
            test->elapsed_ms = 0UL;
            test->report_diag = SP01_PUMP_DIAG_UNKNOWN;
        }
        break;

    case SP01_PUMP_ADC_TEST_RUNNING:
        test->elapsed_ms += SP01_TICK_MS;
        if (pump_diag != SP01_PUMP_DIAG_UNKNOWN ||
            test->elapsed_ms >= SP01_PUMP_ADC_TEST_TIMEOUT_MS) {
            /* 诊断完成后立即停泵，再进入报告阶段。 */
            test->state = SP01_PUMP_ADC_TEST_REPORT;
            test->elapsed_ms = 0UL;
            test->report_diag = pump_diag;
            test->report_beep_pending = APP_TRUE;
        }
        break;

    case SP01_PUMP_ADC_TEST_REPORT:
        if (test->report_beep_pending == APP_TRUE) {
            test->report_beep_pending = APP_FALSE;
            buzzer_mode = sp01_pump_adc_test_report_buzzer(test->report_diag);
        } else if (test->report_diag == SP01_PUMP_DIAG_OVER_CURRENT) {
            /* 过流报告在本轮报告窗口内保持周期蜂鸣，随后自动静音。 */
            buzzer_mode = SP01_BUZZER_FAULT;
        }

        test->elapsed_ms += SP01_TICK_MS;
        if (test->elapsed_ms >= SP01_PUMP_ADC_TEST_REPORT_MS) {
            test->state = SP01_PUMP_ADC_TEST_IDLE;
            test->elapsed_ms = 0UL;
            test->report_diag = SP01_PUMP_DIAG_UNKNOWN;
        }
        break;

    default:
        test->state = SP01_PUMP_ADC_TEST_IDLE;
        test->elapsed_ms = 0UL;
        test->report_beep_pending = APP_FALSE;
        test->report_diag = SP01_PUMP_DIAG_UNKNOWN;
        break;
    }

    test->key_was_pressed = key_pressed;

    if (test->state == SP01_PUMP_ADC_TEST_RUNNING) {
        return sp01_pump_adc_test_outputs(APP_TRUE, SP01_BUZZER_OFF);
    }

    return sp01_pump_adc_test_outputs(APP_FALSE, buzzer_mode);
}
