#include "sp01_battery_pump_test.h"

void sp01_battery_pump_test_init(sp01_battery_pump_test_t *test)
{
    if (test == 0) {
        return;
    }

    /* 专用电池验证版上电后持续开泵。 */
    test->pump_on = APP_TRUE;
    test->elapsed_ms = 0UL;
}

app_bool_t sp01_battery_pump_test_update(sp01_battery_pump_test_t *test)
{
    if (test == 0) {
        return APP_FALSE;
    }

    /* 每次调度都重新钳位为开泵，防止测试过程中被其它流程改低。 */
    test->pump_on = APP_TRUE;
    return APP_TRUE;
}
