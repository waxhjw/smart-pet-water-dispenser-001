#ifndef SP01_POWER_MONITOR_H
#define SP01_POWER_MONITOR_H

#include "app_types.h"

/*
 * 电池 ADC 监测模块。
 *
 * 这里测到的是经过分压后的电池端估算电压，不等同于电量百分比。
 * 模块只负责“电压状态分类”和抗抖，不负责决定产品灯效或是否允许水泵。
 */
typedef enum {
    SP01_POWER_DIAG_UNKNOWN = 0,
    SP01_POWER_DIAG_NORMAL,
    SP01_POWER_DIAG_LOW,
    SP01_POWER_DIAG_CRITICAL,
    SP01_POWER_DIAG_INVALID
} sp01_power_diag_t;

typedef struct {
    app_u16 battery_mv;
    sp01_power_diag_t raw_diag;
    sp01_power_diag_t diag;
    app_u8 bad_confirm_count;
    app_u8 good_confirm_count;
} sp01_power_snapshot_t;

typedef struct {
    app_u16 battery_mv;
    sp01_power_diag_t raw_diag;
    sp01_power_diag_t diag;
    app_u8 bad_confirm_count;
    app_u8 good_confirm_count;
} sp01_power_monitor_t;

void sp01_power_monitor_init(sp01_power_monitor_t *monitor);
sp01_power_diag_t sp01_power_monitor_update(sp01_power_monitor_t *monitor,
                                            app_u16 battery_mv);
sp01_power_snapshot_t sp01_power_monitor_snapshot(const sp01_power_monitor_t *monitor);
const char *sp01_power_diag_name(sp01_power_diag_t diag);

#endif
