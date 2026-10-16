#include <assert.h>

#include "platform/py32f002b/board_port.h"

int main(void)
{
    sp01_outputs_t requested;
    sp01_outputs_t applied;

    board_init();

    requested.pump_enable = APP_TRUE;
    requested.radar_power_enable = APP_TRUE;
    requested.led_mode = SP01_LED_PUMPING;
    requested.buzzer_mode = SP01_BUZZER_KEY_CLICK;
    board_apply_outputs(&requested);

    applied = board_sim_get_outputs();
    assert(applied.pump_enable == APP_FALSE);
    assert(applied.radar_power_enable == APP_TRUE);
    assert(applied.led_mode == SP01_LED_PUMPING);
    assert(applied.buzzer_mode == SP01_BUZZER_KEY_CLICK);

    board_safe_outputs_off();
    applied = board_sim_get_outputs();
    assert(applied.pump_enable == APP_FALSE);

    return 0;
}
