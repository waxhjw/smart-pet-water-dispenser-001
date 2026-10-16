#include "sp01_power_monitor.h"
#include "app_config.h"

static sp01_power_diag_t sp01_power_raw_classify(app_u16 battery_mv)
{
    if (battery_mv == 0U) {
        return SP01_POWER_DIAG_INVALID;
    }

    if (battery_mv <= SP01_BATTERY_CRITICAL_MV) {
        return SP01_POWER_DIAG_CRITICAL;
    }

    if (battery_mv <= SP01_BATTERY_LOW_MV) {
        return SP01_POWER_DIAG_LOW;
    }

    return SP01_POWER_DIAG_NORMAL;
}

static app_bool_t sp01_power_is_bad(sp01_power_diag_t diag)
{
    return (diag == SP01_POWER_DIAG_LOW ||
            diag == SP01_POWER_DIAG_CRITICAL ||
            diag == SP01_POWER_DIAG_INVALID) ? APP_TRUE : APP_FALSE;
}

void sp01_power_monitor_init(sp01_power_monitor_t *monitor)
{
    monitor->battery_mv = 0U;
    monitor->raw_diag = SP01_POWER_DIAG_UNKNOWN;
    monitor->diag = SP01_POWER_DIAG_UNKNOWN;
    monitor->bad_confirm_count = 0U;
    monitor->good_confirm_count = 0U;
}

sp01_power_diag_t sp01_power_monitor_update(sp01_power_monitor_t *monitor,
                                            app_u16 battery_mv)
{
    sp01_power_diag_t raw_diag;

    monitor->battery_mv = battery_mv;
    raw_diag = sp01_power_raw_classify(battery_mv);
    monitor->raw_diag = raw_diag;

    if (sp01_power_is_bad(raw_diag) == APP_TRUE) {
        monitor->good_confirm_count = 0U;
        if (monitor->bad_confirm_count < 255U) {
            monitor->bad_confirm_count++;
        }

        /* 连续多次低压才进入故障，避免单次 ADC 噪声误报。 */
        if (monitor->bad_confirm_count >= SP01_BATTERY_FAULT_CONFIRM_TICKS) {
            monitor->diag = raw_diag;
        }
    } else {
        monitor->bad_confirm_count = 0U;
        if (monitor->good_confirm_count < 255U) {
            monitor->good_confirm_count++;
        }

        /* 恢复也需要连续正常采样，避免刚抬升一拍就清故障。 */
        if (monitor->good_confirm_count >= SP01_BATTERY_RECOVERY_CONFIRM_TICKS) {
            monitor->diag = SP01_POWER_DIAG_NORMAL;
        }
    }

    return monitor->diag;
}

sp01_power_snapshot_t sp01_power_monitor_snapshot(const sp01_power_monitor_t *monitor)
{
    sp01_power_snapshot_t snapshot;

    snapshot.battery_mv = monitor->battery_mv;
    snapshot.raw_diag = monitor->raw_diag;
    snapshot.diag = monitor->diag;
    snapshot.bad_confirm_count = monitor->bad_confirm_count;
    snapshot.good_confirm_count = monitor->good_confirm_count;
    return snapshot;
}

const char *sp01_power_diag_name(sp01_power_diag_t diag)
{
    switch (diag) {
    case SP01_POWER_DIAG_UNKNOWN:
        return "UNKNOWN";
    case SP01_POWER_DIAG_NORMAL:
        return "NORMAL";
    case SP01_POWER_DIAG_LOW:
        return "LOW";
    case SP01_POWER_DIAG_CRITICAL:
        return "CRITICAL";
    case SP01_POWER_DIAG_INVALID:
        return "INVALID";
    default:
        return "INVALID_ENUM";
    }
}
