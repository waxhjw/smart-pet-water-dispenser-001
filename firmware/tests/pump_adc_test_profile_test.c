#include <assert.h>

#include "app/app_config.h"
#include "app/sp01_pump_adc_test.h"

int main(void)
{
    sp01_pump_adc_test_t test;
    sp01_outputs_t outputs;
    app_u32 index;

    sp01_pump_adc_test_init(&test);
    outputs = sp01_pump_adc_test_update(&test, APP_FALSE,
                                        SP01_PUMP_DIAG_UNKNOWN);
    assert(outputs.pump_enable == APP_FALSE);

    /* 短按上升沿启动一轮采样，尚未收到分类时保持开泵。 */
    outputs = sp01_pump_adc_test_update(&test, APP_TRUE,
                                        SP01_PUMP_DIAG_UNKNOWN);
    assert(outputs.pump_enable == APP_TRUE);
    assert(test.state == SP01_PUMP_ADC_TEST_RUNNING);

    /* 收到正常分类后立即停泵，并发出一次性报告请求。 */
    outputs = sp01_pump_adc_test_update(&test, APP_FALSE,
                                        SP01_PUMP_DIAG_NORMAL);
    assert(outputs.pump_enable == APP_FALSE);
    outputs = sp01_pump_adc_test_update(&test, APP_FALSE,
                                        SP01_PUMP_DIAG_UNKNOWN);
    assert(outputs.buzzer_mode == SP01_BUZZER_RADAR_TEST);
    assert(test.state == SP01_PUMP_ADC_TEST_REPORT);

    /* 过流分类在报告窗口内保持故障提示，窗口结束后回到待机。 */
    sp01_pump_adc_test_init(&test);
    (void)sp01_pump_adc_test_update(&test, APP_TRUE,
                                    SP01_PUMP_DIAG_UNKNOWN);
    outputs = sp01_pump_adc_test_update(&test, APP_FALSE,
                                        SP01_PUMP_DIAG_OVER_CURRENT);
    assert(outputs.pump_enable == APP_FALSE);
    outputs = sp01_pump_adc_test_update(&test, APP_FALSE,
                                        SP01_PUMP_DIAG_UNKNOWN);
    assert(outputs.buzzer_mode == SP01_BUZZER_FAULT);
    for (index = 0UL; index < (SP01_PUMP_ADC_TEST_REPORT_MS / SP01_TICK_MS); index++) {
        outputs = sp01_pump_adc_test_update(&test, APP_FALSE,
                                            SP01_PUMP_DIAG_UNKNOWN);
    }
    assert(test.state == SP01_PUMP_ADC_TEST_IDLE);
    assert(outputs.pump_enable == APP_FALSE);

    return 0;
}
