#ifndef SP01_BATTERY_TEST_H
#define SP01_BATTERY_TEST_H

#include "sp01_app.h"
#include "sp01_charge_monitor.h"
#include "sp01_power_monitor.h"

/*
 * 电池台架测试的纯逻辑映射：
 * - 蓝灯：充电输入有效；
 * - 绿灯：充满或电压正常；
 * - 红灯：充电状态冲突或电压低/无效；
 * - 未完成确认时保持熄灭。
 *
 * 该模块只决定指示灯模式，不控制水泵、雷达或充电器。
 */
sp01_led_mode_t sp01_battery_test_led_mode(sp01_power_diag_t power_diag,
                                           const sp01_charge_status_t *charge_status);

#endif
