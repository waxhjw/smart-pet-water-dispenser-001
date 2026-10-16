#ifndef SP01_BUZZER_PATTERN_H
#define SP01_BUZZER_PATTERN_H

#include "app_types.h"
#include "sp01_app.h"

/*
 * 蜂鸣器节奏门控。
 *
 * 产品层只产生“按键/缺水/故障”事件，本模块把事件变成非阻塞的开关节奏。
 * 返回值仍是蜂鸣器模式，底层可以再叠加 PB7 的 PWM 载波。
 */
typedef struct {
    sp01_buzzer_mode_t active_mode;
    app_u32 elapsed_ms;
    app_u16 output_frequency_hz;
    app_u8 output_duty_percent;
} sp01_buzzer_pattern_t;

void sp01_buzzer_pattern_init(sp01_buzzer_pattern_t *pattern);
sp01_buzzer_mode_t sp01_buzzer_pattern_update(sp01_buzzer_pattern_t *pattern,
                                              sp01_buzzer_mode_t requested_mode);

/* 返回当前 10ms 节拍对应的 PWM 频率；静音阶段返回 0。 */
app_u16 sp01_buzzer_pattern_frequency_hz(const sp01_buzzer_pattern_t *pattern);

/* 返回当前提示音的 PWM 占空比百分比；静音阶段返回 0。 */
app_u8 sp01_buzzer_pattern_duty_percent(const sp01_buzzer_pattern_t *pattern);

#endif
