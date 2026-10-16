#ifndef SP01_PUMP_ADC_TEST_H
#define SP01_PUMP_ADC_TEST_H

#include "sp01_app.h"

/*
 * PA3/ADC_XQ 诊断版只做一轮受控采样：短按启动水泵，
 * 到达泵诊断窗口后停泵，再用蜂鸣器报告采样分类。
 */
typedef enum {
    SP01_PUMP_ADC_TEST_IDLE = 0,
    SP01_PUMP_ADC_TEST_RUNNING,
    SP01_PUMP_ADC_TEST_REPORT
} sp01_pump_adc_test_state_t;

typedef struct {
    sp01_pump_adc_test_state_t state;
    app_bool_t key_was_pressed;
    app_bool_t report_beep_pending;
    app_u32 elapsed_ms;
    sp01_pump_diag_t report_diag;
} sp01_pump_adc_test_t;

void sp01_pump_adc_test_init(sp01_pump_adc_test_t *test);
sp01_outputs_t sp01_pump_adc_test_update(sp01_pump_adc_test_t *test,
                                         app_bool_t key_pressed,
                                         sp01_pump_diag_t pump_diag);

#endif
