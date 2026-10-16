#include <assert.h>

#include "app/app_config.h"
#include "app/sp01_battery_pump_test.h"

int main(void)
{
    sp01_battery_pump_test_t test;
    app_u32 i;

    sp01_battery_pump_test_init(&test);

    for (i = 0UL; i < (120000UL / SP01_TICK_MS); i++) {
        assert(sp01_battery_pump_test_update(&test) == APP_TRUE);
        assert(test.pump_on == APP_TRUE);
    }
    return 0;
}
