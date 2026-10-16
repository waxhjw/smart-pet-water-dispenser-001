#include "sp01_signal_filter.h"

void sp01_bool_filter_init(sp01_bool_filter_t *filter, app_bool_t initial_value)
{
    if (filter == 0) {
        return;
    }

    filter->stable = initial_value;
    filter->candidate = initial_value;
    filter->candidate_ticks = 0U;
}

app_bool_t sp01_bool_filter_update(sp01_bool_filter_t *filter,
                                   app_bool_t sample,
                                   app_u8 required_ticks)
{
    if (filter == 0) {
        return APP_FALSE;
    }

    if (required_ticks <= 1U) {
        filter->stable = sample;
        filter->candidate = sample;
        filter->candidate_ticks = 0U;
        return filter->stable;
    }

    if (sample == filter->stable) {
        filter->candidate = sample;
        filter->candidate_ticks = 0U;
        return filter->stable;
    }

    if (sample != filter->candidate) {
        filter->candidate = sample;
        filter->candidate_ticks = 1U;
    } else if (filter->candidate_ticks < 0xFFU) {
        filter->candidate_ticks++;
    }

    if (filter->candidate_ticks >= required_ticks) {
        filter->stable = filter->candidate;
        filter->candidate_ticks = 0U;
    }

    return filter->stable;
}
