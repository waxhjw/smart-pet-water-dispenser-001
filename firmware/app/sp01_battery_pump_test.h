#ifndef SP01_BATTERY_PUMP_TEST_H
#define SP01_BATTERY_PUMP_TEST_H

#include "app_types.h"

/*
 * 电池供电水泵常高台架测试：上电后持续保持 PB2 高电平。
 * 这是专用验证固件，复位初始化阶段也由板级代码保持水泵输出有效。
 */
typedef struct {
    app_bool_t pump_on;
    app_u32 elapsed_ms;
} sp01_battery_pump_test_t;

void sp01_battery_pump_test_init(sp01_battery_pump_test_t *test);
app_bool_t sp01_battery_pump_test_update(sp01_battery_pump_test_t *test);

#endif
