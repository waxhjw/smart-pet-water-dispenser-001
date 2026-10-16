#include "sp01_radar_input.h"

void sp01_radar_input_init(sp01_radar_input_t *input,
                           app_bool_t initial_active)
{
    if (input == 0) {
        return;
    }

    sp01_bool_filter_init(&input->filter, initial_active);
    input->stable_active = initial_active;
}

sp01_radar_event_t sp01_radar_input_update(sp01_radar_input_t *input,
                                           app_bool_t raw_active,
                                           app_u8 debounce_ticks)
{
    sp01_radar_event_t event;
    app_bool_t stable_active;

    event.active = APP_FALSE;
    event.rising_edge = APP_FALSE;
    event.falling_edge = APP_FALSE;

    if (input == 0) {
        return event;
    }

    stable_active = sp01_bool_filter_update(&input->filter,
                                            raw_active,
                                            debounce_ticks);
    event.active = stable_active;

    if (stable_active != input->stable_active) {
        if (stable_active == APP_TRUE) {
            event.rising_edge = APP_TRUE;
        } else {
            event.falling_edge = APP_TRUE;
        }
        input->stable_active = stable_active;
    }

    return event;
}
