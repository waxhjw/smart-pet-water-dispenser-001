#include <assert.h>

#include "app/app_config.h"
#include "app/sp01_pump_test.h"

static app_bool_t run_ticks(sp01_pump_test_t *test,
                            app_bool_t key_pressed,
                            app_u32 tick_count)
{
    app_u32 index;
    app_bool_t pump_enabled = APP_FALSE;

    for (index = 0UL; index < tick_count; index++) {
        pump_enabled = sp01_pump_test_update(test,
                                             key_pressed,
                                             SP01_TICK_MS,
                                             SP01_PUMP_TEST_RELEASE_ARM_MS,
                                             SP01_PUMP_TEST_LONG_PRESS_MS,
                                             SP01_PUMP_TEST_RUN_MS);
    }

    return pump_enabled;
}

int main(void)
{
    sp01_pump_test_t test;
    sp01_pump_test_t abort_test;
    app_u32 index;
    app_bool_t pump_enabled;

    sp01_pump_test_init(&test);

    assert(run_ticks(&test, APP_TRUE, 300UL) == APP_FALSE);
    assert(test.state == SP01_PUMP_TEST_WAIT_RELEASE);
    assert(run_ticks(&test, APP_FALSE,
                     SP01_PUMP_TEST_RELEASE_ARM_MS / SP01_TICK_MS) == APP_FALSE);
    assert(test.state == SP01_PUMP_TEST_ARMED);

    assert(run_ticks(&test, APP_TRUE, 10UL) == APP_FALSE);
    assert(run_ticks(&test, APP_FALSE, 1UL) == APP_FALSE);
    assert(test.state == SP01_PUMP_TEST_ARMED);

    for (index = 0UL; index < (SP01_PUMP_TEST_LONG_PRESS_MS / SP01_TICK_MS) - 1UL; index++) {
        assert(sp01_pump_test_update(&test,
                                     APP_TRUE,
                                     SP01_TICK_MS,
                                     SP01_PUMP_TEST_RELEASE_ARM_MS,
                                     SP01_PUMP_TEST_LONG_PRESS_MS,
                                     SP01_PUMP_TEST_RUN_MS) == APP_FALSE);
    }
    assert(sp01_pump_test_update(&test,
                                 APP_TRUE,
                                 SP01_TICK_MS,
                                 SP01_PUMP_TEST_RELEASE_ARM_MS,
                                 SP01_PUMP_TEST_LONG_PRESS_MS,
                                 SP01_PUMP_TEST_RUN_MS) == APP_TRUE);

    for (index = 0UL; index < (SP01_PUMP_TEST_RUN_MS / SP01_TICK_MS) - 1UL; index++) {
        assert(sp01_pump_test_update(&test,
                                     APP_TRUE,
                                     SP01_TICK_MS,
                                     SP01_PUMP_TEST_RELEASE_ARM_MS,
                                     SP01_PUMP_TEST_LONG_PRESS_MS,
                                     SP01_PUMP_TEST_RUN_MS) == APP_TRUE);
    }
    assert(sp01_pump_test_update(&test,
                                 APP_TRUE,
                                 SP01_TICK_MS,
                                 SP01_PUMP_TEST_RELEASE_ARM_MS,
                                 SP01_PUMP_TEST_LONG_PRESS_MS,
                                 SP01_PUMP_TEST_RUN_MS) == APP_FALSE);
    assert(test.state == SP01_PUMP_TEST_DONE);

    assert(run_ticks(&test, APP_FALSE, 100UL) == APP_FALSE);
    assert(run_ticks(&test, APP_TRUE, 300UL) == APP_FALSE);
    assert(test.state == SP01_PUMP_TEST_DONE);

    sp01_pump_test_init(&abort_test);
    assert(run_ticks(&abort_test, APP_FALSE,
                     SP01_PUMP_TEST_RELEASE_ARM_MS / SP01_TICK_MS) == APP_FALSE);
    for (index = 0UL; index < SP01_PUMP_TEST_LONG_PRESS_MS / SP01_TICK_MS; index++) {
        pump_enabled = sp01_pump_test_update(&abort_test,
                                             APP_TRUE,
                                             SP01_TICK_MS,
                                             SP01_PUMP_TEST_RELEASE_ARM_MS,
                                             SP01_PUMP_TEST_LONG_PRESS_MS,
                                             SP01_PUMP_TEST_RUN_MS);
    }
    assert(pump_enabled == APP_TRUE);
    assert(sp01_pump_test_update(&abort_test,
                                 APP_FALSE,
                                 SP01_TICK_MS,
                                 SP01_PUMP_TEST_RELEASE_ARM_MS,
                                 SP01_PUMP_TEST_LONG_PRESS_MS,
                                 SP01_PUMP_TEST_RUN_MS) == APP_TRUE);
    assert(sp01_pump_test_update(&abort_test,
                                 APP_TRUE,
                                 SP01_TICK_MS,
                                 SP01_PUMP_TEST_RELEASE_ARM_MS,
                                 SP01_PUMP_TEST_LONG_PRESS_MS,
                                 SP01_PUMP_TEST_RUN_MS) == APP_FALSE);
    assert(abort_test.state == SP01_PUMP_TEST_DONE);

    return 0;
}
