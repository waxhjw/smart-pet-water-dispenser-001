#include "sp01_charge_monitor.h"

void sp01_charge_monitor_init(sp01_charge_monitor_t *monitor)
{
    if (monitor == 0) {
        return;
    }

    sp01_bool_filter_init(&monitor->charging_filter, APP_FALSE);
    sp01_bool_filter_init(&monitor->done_filter, APP_FALSE);
    monitor->status.charging_active = APP_FALSE;
    monitor->status.charge_done = APP_FALSE;
    monitor->status.conflict = APP_FALSE;
}

sp01_charge_status_t sp01_charge_monitor_update(sp01_charge_monitor_t *monitor,
                                                 app_bool_t raw_charging_active,
                                                 app_bool_t raw_charge_done,
                                                 app_u8 debounce_ticks)
{
    sp01_charge_status_t status;

    status.charging_active = APP_FALSE;
    status.charge_done = APP_FALSE;
    status.conflict = APP_FALSE;

    if (monitor == 0) {
        return status;
    }

    monitor->status.charging_active =
        sp01_bool_filter_update(&monitor->charging_filter,
                                raw_charging_active,
                                debounce_ticks);
    monitor->status.charge_done =
        sp01_bool_filter_update(&monitor->done_filter,
                                raw_charge_done,
                                debounce_ticks);
    monitor->status.conflict =
        (monitor->status.charging_active == APP_TRUE &&
         monitor->status.charge_done == APP_TRUE) ? APP_TRUE : APP_FALSE;

    return monitor->status;
}
