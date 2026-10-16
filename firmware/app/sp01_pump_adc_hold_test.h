#ifndef SP01_PUMP_ADC_HOLD_TEST_H
#define SP01_PUMP_ADC_HOLD_TEST_H

#include "sp01_app.h"

typedef struct {
    app_bool_t pump_enabled;
    app_bool_t key_was_pressed;
    app_bool_t armed;
} sp01_pump_adc_hold_test_t;

void sp01_pump_adc_hold_test_init(sp01_pump_adc_hold_test_t *test);
sp01_outputs_t sp01_pump_adc_hold_test_update(sp01_pump_adc_hold_test_t *test,
                                              app_bool_t key_pressed,
                                              app_bool_t raw_key_pressed);

#endif
