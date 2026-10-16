#ifndef SP01_SIGNAL_FILTER_H
#define SP01_SIGNAL_FILTER_H

#include "app_types.h"

/*
 * 通用布尔输入去抖器。
 * 每次调用代表一个固定节拍（当前为 10ms），连续达到 required_ticks 后才接受新电平。
 */
typedef struct {
    app_bool_t stable;
    app_bool_t candidate;
    app_u8 candidate_ticks;
} sp01_bool_filter_t;

void sp01_bool_filter_init(sp01_bool_filter_t *filter, app_bool_t initial_value);
app_bool_t sp01_bool_filter_update(sp01_bool_filter_t *filter,
                                   app_bool_t sample,
                                   app_u8 required_ticks);

#endif
