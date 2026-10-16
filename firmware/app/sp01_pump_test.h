#ifndef SP01_PUMP_TEST_H
#define SP01_PUMP_TEST_H

#include "app_types.h"

typedef enum {
    SP01_PUMP_TEST_WAIT_RELEASE = 0,
    SP01_PUMP_TEST_ARMED,
    SP01_PUMP_TEST_RUNNING,
    SP01_PUMP_TEST_DONE
} sp01_pump_test_state_t;

typedef struct {
    sp01_pump_test_state_t state;
    app_u32 elapsed_ms;
    app_bool_t stop_armed;
} sp01_pump_test_t;

void sp01_pump_test_init(sp01_pump_test_t *test);
app_bool_t sp01_pump_test_update(sp01_pump_test_t *test,
                                 app_bool_t key_pressed,
                                 app_u32 tick_ms,
                                 app_u32 release_arm_ms,
                                 app_u32 long_press_ms,
                                 app_u32 run_ms);

#endif
