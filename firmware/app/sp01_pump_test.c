
#include "sp01_pump_test.h"

static app_u32 sp01_pump_test_accumulate(app_u32 elapsed_ms,
                                         app_u32 tick_ms,
                                         app_u32 limit_ms)
{
    if (elapsed_ms >= limit_ms || tick_ms >= (limit_ms - elapsed_ms)) {
        return limit_ms;
    }

    return elapsed_ms + tick_ms;
}

void sp01_pump_test_init(sp01_pump_test_t *test)
{
    if (test == 0) {
        return;
    }

    test->state = SP01_PUMP_TEST_WAIT_RELEASE;
    test->elapsed_ms = 0UL;
    test->stop_armed = APP_FALSE;
}

app_bool_t sp01_pump_test_update(sp01_pump_test_t *test,
                                 app_bool_t key_pressed,
                                 app_u32 tick_ms,
                                 app_u32 release_arm_ms,
                                 app_u32 long_press_ms,
                                 app_u32 run_ms)
{
    if (test == 0 || tick_ms == 0UL || run_ms == 0UL) {
        return APP_FALSE;
    }

    switch (test->state) {
    case SP01_PUMP_TEST_WAIT_RELEASE:
        if (key_pressed == APP_TRUE) {
            test->elapsed_ms = 0UL;
        } else {
            test->elapsed_ms = sp01_pump_test_accumulate(test->elapsed_ms,
                                                         tick_ms,
                                                         release_arm_ms);
            if (test->elapsed_ms >= release_arm_ms) {
                test->state = SP01_PUMP_TEST_ARMED;
                test->elapsed_ms = 0UL;
            }
        }
        return APP_FALSE;

    case SP01_PUMP_TEST_ARMED:
        if (key_pressed == APP_FALSE) {
            test->elapsed_ms = 0UL;
            return APP_FALSE;
        }

        test->elapsed_ms = sp01_pump_test_accumulate(test->elapsed_ms,
                                                     tick_ms,
                                                     long_press_ms);
        if (test->elapsed_ms >= long_press_ms) {
            test->state = SP01_PUMP_TEST_RUNNING;
            test->elapsed_ms = sp01_pump_test_accumulate(0UL,
                                                         tick_ms,
                                                         run_ms);
            test->stop_armed = APP_FALSE;
            return APP_TRUE;
        } else {
            return APP_FALSE;
        }

    case SP01_PUMP_TEST_RUNNING:
        if (key_pressed == APP_FALSE) {
            test->stop_armed = APP_TRUE;
        } else if (test->stop_armed == APP_TRUE) {
            test->state = SP01_PUMP_TEST_DONE;
            return APP_FALSE;
        }
        if (test->elapsed_ms >= run_ms) {
            test->state = SP01_PUMP_TEST_DONE;
            return APP_FALSE;
        }
        test->elapsed_ms = sp01_pump_test_accumulate(test->elapsed_ms,
                                                     tick_ms,
                                                     run_ms);
        return APP_TRUE;

    case SP01_PUMP_TEST_DONE:
    default:
        return APP_FALSE;
    }
}
