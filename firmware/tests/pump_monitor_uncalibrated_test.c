#include <assert.h>

#include "app/app_config.h"
#include "app/sp01_pump_monitor.h"

int main(void)
{
    sp01_pump_monitor_t monitor;
    sp01_pump_snapshot_t snapshot;
    app_u32 ticks;
    app_u32 index;

    sp01_pump_monitor_init(&monitor);
    ticks = (SP01_PUMP_START_IGNORE_MS + SP01_PUMP_DIAG_WINDOW_MS) /
            SP01_TICK_MS;

    for (index = 0U; index < ticks; index++) {
        assert(sp01_pump_monitor_update(&monitor, APP_TRUE, 100U) ==
               SP01_PUMP_DIAG_UNKNOWN);
    }

    snapshot = sp01_pump_monitor_snapshot(&monitor, 100U);
    assert(snapshot.diag == SP01_PUMP_DIAG_UNKNOWN);
    assert(snapshot.sample_count == SP01_PUMP_DIAG_WINDOW_MS / SP01_TICK_MS);
    assert(snapshot.avg_mv == 100U);

    return 0;
}
