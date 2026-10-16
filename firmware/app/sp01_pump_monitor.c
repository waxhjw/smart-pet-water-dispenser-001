#include "sp01_pump_monitor.h"
#include "app_config.h"

static void sp01_pump_monitor_reset_window(sp01_pump_monitor_t *monitor)
{
    monitor->min_mv = 0xFFFFU;
    monitor->max_mv = 0U;
    monitor->sum_mv = 0UL;
    monitor->sample_count = 0U;
}

static app_u16 sp01_pump_monitor_avg(const sp01_pump_monitor_t *monitor)
{
    if (monitor->sample_count == 0U) {
        return 0U;
    }

    return (app_u16)(monitor->sum_mv / monitor->sample_count);
}

#if SP01_PUMP_DIAGNOSTICS_ENABLED
static sp01_pump_diag_t sp01_pump_monitor_classify(const sp01_pump_monitor_t *monitor)
{
    app_u16 avg_mv;

    avg_mv = sp01_pump_monitor_avg(monitor);

    if (monitor->sample_count == 0U) {
        return SP01_PUMP_DIAG_UNKNOWN;
    }

    if (avg_mv >= SP01_PUMP_ADC_OVER_CURRENT_MV) {
        return SP01_PUMP_DIAG_OVER_CURRENT;
    }

    if (avg_mv < SP01_PUMP_ADC_PRESENT_MIN_MV) {
        return SP01_PUMP_DIAG_NO_LOAD;
    }

    if (avg_mv >= SP01_PUMP_ADC_LOW_WATER_MIN_MV &&
        avg_mv <= SP01_PUMP_ADC_LOW_WATER_MAX_MV) {
        return SP01_PUMP_DIAG_LOW_WATER_SUSPECT;
    }

    return SP01_PUMP_DIAG_NORMAL;
}
#endif

void sp01_pump_monitor_init(sp01_pump_monitor_t *monitor)
{
    monitor->pump_was_enabled = APP_FALSE;
    monitor->pump_on_elapsed_ms = 0UL;
    monitor->off_baseline_mv = 0U;
    monitor->diag = SP01_PUMP_DIAG_UNKNOWN;
    sp01_pump_monitor_reset_window(monitor);
}

sp01_pump_diag_t sp01_pump_monitor_update(sp01_pump_monitor_t *monitor,
                                          app_bool_t pump_enabled,
                                          app_u16 adc_mv)
{
    if (pump_enabled == APP_FALSE) {
        /* 关泵时记录零点/底噪，给后续调试看，不直接参与最终判定。 */
        monitor->pump_was_enabled = APP_FALSE;
        monitor->pump_on_elapsed_ms = 0UL;
        monitor->off_baseline_mv = adc_mv;
        monitor->diag = SP01_PUMP_DIAG_UNKNOWN;
        sp01_pump_monitor_reset_window(monitor);
        return monitor->diag;
    }

    if (monitor->pump_was_enabled == APP_FALSE) {
        monitor->pump_was_enabled = APP_TRUE;
        monitor->pump_on_elapsed_ms = 0UL;
        monitor->diag = SP01_PUMP_DIAG_UNKNOWN;
        sp01_pump_monitor_reset_window(monitor);
    }

    monitor->pump_on_elapsed_ms += SP01_TICK_MS;

    /* 刚开泵时无线发射模块会有启动过程，先不判缺水。 */
    if (monitor->pump_on_elapsed_ms <= SP01_PUMP_START_IGNORE_MS) {
        monitor->diag = SP01_PUMP_DIAG_UNKNOWN;
        return monitor->diag;
    }

    if (monitor->sample_count == 0U) {
        monitor->min_mv = adc_mv;
        monitor->max_mv = adc_mv;
    } else {
        if (adc_mv < monitor->min_mv) {
            monitor->min_mv = adc_mv;
        }
        if (adc_mv > monitor->max_mv) {
            monitor->max_mv = adc_mv;
        }
    }

    if (monitor->sample_count < 0xFFFFU) {
        monitor->sample_count++;
        monitor->sum_mv += adc_mv;
    }

    if (monitor->pump_on_elapsed_ms < (SP01_PUMP_START_IGNORE_MS + SP01_PUMP_DIAG_WINDOW_MS)) {
        monitor->diag = SP01_PUMP_DIAG_UNKNOWN;
        return monitor->diag;
    }

#if SP01_PUMP_DIAGNOSTICS_ENABLED
    monitor->diag = sp01_pump_monitor_classify(monitor);
#else
    monitor->diag = SP01_PUMP_DIAG_UNKNOWN;
#endif
    return monitor->diag;
}

sp01_pump_snapshot_t sp01_pump_monitor_snapshot(const sp01_pump_monitor_t *monitor,
                                                app_u16 adc_mv)
{
    sp01_pump_snapshot_t snapshot;

    snapshot.adc_mv = adc_mv;
    snapshot.avg_mv = sp01_pump_monitor_avg(monitor);
    snapshot.min_mv = monitor->sample_count == 0U ? 0U : monitor->min_mv;
    snapshot.max_mv = monitor->sample_count == 0U ? 0U : monitor->max_mv;
    snapshot.ripple_mv = snapshot.max_mv >= snapshot.min_mv ?
                         (app_u16)(snapshot.max_mv - snapshot.min_mv) : 0U;
    snapshot.sample_count = monitor->sample_count;
    snapshot.diag = monitor->diag;

    return snapshot;
}
