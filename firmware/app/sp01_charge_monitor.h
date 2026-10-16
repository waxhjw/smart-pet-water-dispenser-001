#ifndef SP01_CHARGE_MONITOR_H
#define SP01_CHARGE_MONITOR_H

#include "app_types.h"
#include "sp01_signal_filter.h"

/*
 * 充电状态是两个独立的逻辑输入：
 * - charging_active：充电器正在给电池充电
 * - charge_done：充电器判断电池已充满
 *
 * conflict 表示两个信号经过去抖后同时有效。它不是自动故障判定，先保留给
 * 首板调试观察和后续硬件确认，避免在 CHRG/DONE 极性或时序未确认前误锁机器。
 */
typedef struct {
    app_bool_t charging_active;
    app_bool_t charge_done;
    app_bool_t conflict;
} sp01_charge_status_t;

typedef struct {
    sp01_bool_filter_t charging_filter;
    sp01_bool_filter_t done_filter;
    sp01_charge_status_t status;
} sp01_charge_monitor_t;

void sp01_charge_monitor_init(sp01_charge_monitor_t *monitor);
sp01_charge_status_t sp01_charge_monitor_update(sp01_charge_monitor_t *monitor,
                                                 app_bool_t raw_charging_active,
                                                 app_bool_t raw_charge_done,
                                                 app_u8 debounce_ticks);

#endif
