#include "sp01_buzzer_pattern.h"
#include "app_config.h"

typedef struct {
    app_u32 duration_ms;
    app_bool_t on;
    app_u16 frequency_hz;
    app_u8 duty_percent;
} sp01_buzzer_segment_t;

static app_u8 sp01_buzzer_segment_count(sp01_buzzer_mode_t mode)
{
    switch (mode) {
    case SP01_BUZZER_POWER_ON:
    case SP01_BUZZER_POWER_OFF:
        return 5U;
    case SP01_BUZZER_DOUBLE_CLICK:
    case SP01_BUZZER_NIGHT:
        return 3U;
    case SP01_BUZZER_KEY_CLICK:
    case SP01_BUZZER_RADAR_TEST:
    case SP01_BUZZER_LOW_WATER:
    case SP01_BUZZER_ERROR:
        return 1U;
    case SP01_BUZZER_FAULT:
        return 2U;
    case SP01_BUZZER_OFF:
    default:
        return 0U;
    }
}

static sp01_buzzer_segment_t sp01_buzzer_segment(sp01_buzzer_mode_t mode,
                                                  app_u8 index)
{
    sp01_buzzer_segment_t segment;

    segment.duration_ms = 0UL;
    segment.on = APP_FALSE;
    segment.frequency_hz = 0U;
    segment.duty_percent = 0U;

    switch (mode) {
    case SP01_BUZZER_POWER_ON:
        if (index == 0U) {
            segment.duration_ms = SP01_FULL_TEST_BEEP_POWER_ON_1_MS;
            segment.on = APP_TRUE;
            segment.frequency_hz = 2000U;
            segment.duty_percent = SP01_POWER_ON_DUTY_1_PERCENT;
        } else if (index == 1U) {
            segment.duration_ms = SP01_FULL_TEST_BEEP_POWER_ON_PAUSE_MS;
        } else if (index == 2U) {
            segment.duration_ms = SP01_FULL_TEST_BEEP_POWER_ON_1_MS;
            segment.on = APP_TRUE;
            segment.frequency_hz = 2500U;
            segment.duty_percent = SP01_POWER_ON_DUTY_2_PERCENT;
        } else if (index == 3U) {
            segment.duration_ms = SP01_FULL_TEST_BEEP_POWER_ON_PAUSE_MS;
        } else {
            segment.duration_ms = SP01_FULL_TEST_BEEP_POWER_ON_2_MS;
            segment.on = APP_TRUE;
            segment.frequency_hz = 3000U;
            segment.duty_percent = SP01_POWER_ON_DUTY_3_PERCENT;
        }
        break;

    case SP01_BUZZER_POWER_OFF:
        if (index == 0U) {
            segment.duration_ms = SP01_FULL_TEST_BEEP_POWER_OFF_1_MS;
            segment.on = APP_TRUE;
            segment.frequency_hz = 3000U;
            segment.duty_percent = SP01_POWER_OFF_DUTY_1_PERCENT;
        } else if (index == 1U) {
            segment.duration_ms = SP01_FULL_TEST_BEEP_POWER_ON_PAUSE_MS;
        } else if (index == 2U) {
            segment.duration_ms = SP01_FULL_TEST_BEEP_POWER_OFF_1_MS;
            segment.on = APP_TRUE;
            segment.frequency_hz = 2500U;
            segment.duty_percent = SP01_POWER_OFF_DUTY_2_PERCENT;
        } else if (index == 3U) {
            segment.duration_ms = SP01_FULL_TEST_BEEP_POWER_ON_PAUSE_MS;
        } else {
            segment.duration_ms = SP01_FULL_TEST_BEEP_POWER_OFF_2_MS;
            segment.on = APP_TRUE;
            segment.frequency_hz = 2000U;
            segment.duty_percent = SP01_POWER_OFF_DUTY_3_PERCENT;
        }
        break;

    case SP01_BUZZER_DOUBLE_CLICK:
        if (index == 0U || index == 2U) {
            segment.duration_ms = SP01_FULL_TEST_BEEP_SHORT_MS;
            segment.on = APP_TRUE;
            segment.frequency_hz = 1800U;
            segment.duty_percent = SP01_BUZZER_DEFAULT_DUTY_PERCENT;
        } else {
            segment.duration_ms = SP01_FULL_TEST_BEEP_PAUSE_MS;
        }
        break;

    case SP01_BUZZER_NIGHT:
        if (index == 0U || index == 2U) {
            segment.duration_ms = SP01_FULL_TEST_BEEP_NIGHT_MS;
            segment.on = APP_TRUE;
            segment.frequency_hz = 3200U;
            segment.duty_percent = SP01_BUZZER_DEFAULT_DUTY_PERCENT;
        } else {
            segment.duration_ms = SP01_FULL_TEST_BEEP_PAUSE_MS;
        }
        break;

    case SP01_BUZZER_KEY_CLICK:
        segment.duration_ms = SP01_BUZZER_KEY_CLICK_MS;
        segment.on = APP_TRUE;
        segment.frequency_hz = 1800U;
        segment.duty_percent = SP01_BUZZER_DEFAULT_DUTY_PERCENT;
        break;

    case SP01_BUZZER_RADAR_TEST:
        segment.duration_ms = SP01_BUZZER_RADAR_TEST_MS;
        segment.on = APP_TRUE;
        segment.frequency_hz = 2400U;
        segment.duty_percent = SP01_BUZZER_DEFAULT_DUTY_PERCENT;
        break;

    case SP01_BUZZER_LOW_WATER:
        segment.duration_ms = SP01_BUZZER_LOW_WATER_MS;
        segment.on = APP_TRUE;
        segment.frequency_hz = 1000U;
        segment.duty_percent = SP01_BUZZER_DEFAULT_DUTY_PERCENT;
        break;

    case SP01_BUZZER_ERROR:
        segment.duration_ms = 120UL;
        segment.on = APP_TRUE;
        segment.frequency_hz = 600U;
        segment.duty_percent = SP01_BUZZER_DEFAULT_DUTY_PERCENT;
        break;

    case SP01_BUZZER_FAULT:
        if (index == 0U) {
            segment.duration_ms = SP01_BUZZER_FAULT_ON_MS;
            segment.on = APP_TRUE;
            segment.frequency_hz = 800U;
            segment.duty_percent = SP01_BUZZER_DEFAULT_DUTY_PERCENT;
        } else {
            segment.duration_ms = SP01_BUZZER_FAULT_PERIOD_MS -
                                  SP01_BUZZER_FAULT_ON_MS;
        }
        break;

    case SP01_BUZZER_OFF:
    default:
        break;
    }

    return segment;
}

static app_u32 sp01_buzzer_total_duration(sp01_buzzer_mode_t mode)
{
    app_u8 i;
    app_u8 count;
    app_u32 total;

    count = sp01_buzzer_segment_count(mode);
    total = 0UL;
    for (i = 0U; i < count; i++) {
        total += sp01_buzzer_segment(mode, i).duration_ms;
    }
    return total;
}

static sp01_buzzer_segment_t sp01_buzzer_current_segment(
    const sp01_buzzer_pattern_t *pattern)
{
    app_u8 i;
    app_u8 count;
    app_u32 elapsed;
    sp01_buzzer_segment_t segment;

    segment.duration_ms = 0UL;
    segment.on = APP_FALSE;
    segment.frequency_hz = 0U;
    segment.duty_percent = 0U;
    if (pattern == 0 || pattern->active_mode == SP01_BUZZER_OFF) {
        return segment;
    }

    count = sp01_buzzer_segment_count(pattern->active_mode);
    elapsed = pattern->elapsed_ms;
    for (i = 0U; i < count; i++) {
        segment = sp01_buzzer_segment(pattern->active_mode, i);
        if (elapsed < segment.duration_ms) {
            return segment;
        }
        elapsed -= segment.duration_ms;
    }

    return segment;
}

void sp01_buzzer_pattern_init(sp01_buzzer_pattern_t *pattern)
{
    pattern->active_mode = SP01_BUZZER_OFF;
    pattern->elapsed_ms = 0UL;
    pattern->output_frequency_hz = 0U;
    pattern->output_duty_percent = 0U;
}

sp01_buzzer_mode_t sp01_buzzer_pattern_update(sp01_buzzer_pattern_t *pattern,
                                              sp01_buzzer_mode_t requested_mode)
{
    sp01_buzzer_segment_t segment;
    app_u32 total_duration_ms;

    /* 新事件到来时重新开始节奏；OFF 不会打断尚未结束的一次性提示。 */
    if (requested_mode != SP01_BUZZER_OFF &&
        requested_mode != pattern->active_mode) {
        pattern->active_mode = requested_mode;
        pattern->elapsed_ms = 0UL;
    } else if (requested_mode == SP01_BUZZER_OFF &&
               pattern->active_mode == SP01_BUZZER_FAULT) {
        /* 故障离开后立即停止重复提示。 */
        pattern->active_mode = SP01_BUZZER_OFF;
        pattern->elapsed_ms = 0UL;
    }

    if (pattern->active_mode == SP01_BUZZER_OFF) {
        pattern->output_frequency_hz = 0U;
        pattern->output_duty_percent = 0U;
        return SP01_BUZZER_OFF;
    }

    segment = sp01_buzzer_current_segment(pattern);
    total_duration_ms = sp01_buzzer_total_duration(pattern->active_mode);

    /* 输出参数对应当前 10ms 节拍，避免 elapsed_ms 前进后提前切到下一段。 */
    pattern->output_frequency_hz = segment.on == APP_TRUE ?
                                   segment.frequency_hz : 0U;
    pattern->output_duty_percent = segment.on == APP_TRUE ?
                                   segment.duty_percent : 0U;

    if (pattern->active_mode == SP01_BUZZER_FAULT) {
        pattern->elapsed_ms += SP01_TICK_MS;
        if (pattern->elapsed_ms >= total_duration_ms) {
            pattern->elapsed_ms = 0UL;
        }
        return segment.on == APP_TRUE ? SP01_BUZZER_FAULT : SP01_BUZZER_OFF;
    }

    if (total_duration_ms == 0UL || pattern->elapsed_ms >= total_duration_ms) {
        pattern->active_mode = SP01_BUZZER_OFF;
        pattern->elapsed_ms = 0UL;
        pattern->output_frequency_hz = 0U;
        pattern->output_duty_percent = 0U;
        return SP01_BUZZER_OFF;
    }

    pattern->elapsed_ms += SP01_TICK_MS;
    return segment.on == APP_TRUE ? pattern->active_mode : SP01_BUZZER_OFF;
}

app_u16 sp01_buzzer_pattern_frequency_hz(const sp01_buzzer_pattern_t *pattern)
{
    sp01_buzzer_segment_t segment;

    if (pattern == 0) {
        return 0U;
    }
    if (pattern->active_mode == SP01_BUZZER_POWER_ON ||
        pattern->active_mode == SP01_BUZZER_POWER_OFF) {
        return pattern->output_frequency_hz;
    }

    segment = sp01_buzzer_current_segment(pattern);
    return segment.on == APP_TRUE ? segment.frequency_hz : 0U;
}

app_u8 sp01_buzzer_pattern_duty_percent(const sp01_buzzer_pattern_t *pattern)
{
    sp01_buzzer_segment_t segment;

    if (pattern == 0) {
        return 0U;
    }
    if (pattern->active_mode == SP01_BUZZER_POWER_ON ||
        pattern->active_mode == SP01_BUZZER_POWER_OFF) {
        return pattern->output_duty_percent;
    }

    segment = sp01_buzzer_current_segment(pattern);
    return segment.on == APP_TRUE ? segment.duty_percent : 0U;
}
