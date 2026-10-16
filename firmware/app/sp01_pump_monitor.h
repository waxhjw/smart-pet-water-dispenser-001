#ifndef SP01_PUMP_MONITOR_H
#define SP01_PUMP_MONITOR_H

#include "app_types.h"
#include "sp01_app.h"

/*
 * 无线泵低水位检测不是直接测水位，而是观察无线发射模块输入电流。
 * 这个模块只做特征提取和初步诊断，最终阈值必须用样机实测修正。
 */

typedef struct {
    app_bool_t pump_was_enabled;
    app_u32 pump_on_elapsed_ms;
    app_u16 off_baseline_mv;
    app_u16 min_mv;
    app_u16 max_mv;
    app_u32 sum_mv;
    app_u16 sample_count;
    sp01_pump_diag_t diag;
} sp01_pump_monitor_t;

typedef struct {
    app_u16 adc_mv;
    app_u16 avg_mv;
    app_u16 min_mv;
    app_u16 max_mv;
    app_u16 ripple_mv;
    app_u16 sample_count;
    sp01_pump_diag_t diag;
} sp01_pump_snapshot_t;

void sp01_pump_monitor_init(sp01_pump_monitor_t *monitor);
sp01_pump_diag_t sp01_pump_monitor_update(sp01_pump_monitor_t *monitor,
                                          app_bool_t pump_enabled,
                                          app_u16 adc_mv);
sp01_pump_snapshot_t sp01_pump_monitor_snapshot(const sp01_pump_monitor_t *monitor,
                                                app_u16 adc_mv);

#endif
