#ifndef SP01_RADAR_INPUT_H
#define SP01_RADAR_INPUT_H

#include "app_types.h"
#include "sp01_signal_filter.h"

/*
 * 雷达输入事件。
 * active 是去抖后的稳定电平；rising_edge/falling_edge 只在稳定电平真正变化
 * 的那个 10ms tick 产生一次，方便后续扩展“刚进入感应区/刚离开感应区”逻辑。
 */
typedef struct {
    app_bool_t active;
    app_bool_t rising_edge;
    app_bool_t falling_edge;
} sp01_radar_event_t;

typedef struct {
    sp01_bool_filter_t filter;
    app_bool_t stable_active;
} sp01_radar_input_t;

void sp01_radar_input_init(sp01_radar_input_t *input,
                           app_bool_t initial_active);
sp01_radar_event_t sp01_radar_input_update(sp01_radar_input_t *input,
                                           app_bool_t raw_active,
                                           app_u8 debounce_ticks);

#endif
