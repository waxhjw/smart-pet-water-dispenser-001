#include "sp01_battery_test.h"

sp01_led_mode_t sp01_battery_test_led_mode(sp01_power_diag_t power_diag,
                                           const sp01_charge_status_t *charge_status)
{
    if (charge_status == 0) {
        return SP01_LED_FAULT;
    }

    /* CHRG 和 DONE 同时有效通常表示接线、极性或充电器状态异常。 */
    if (charge_status->conflict == APP_TRUE) {
        return SP01_LED_FAULT;
    }

    /* 充电时优先显示充电状态；电压采样仍保留在 SWD 快照中。 */
    if (charge_status->charging_active == APP_TRUE) {
        return SP01_LED_CHARGING;
    }

    if (charge_status->charge_done == APP_TRUE) {
        return SP01_LED_FULL;
    }

    switch (power_diag) {
    case SP01_POWER_DIAG_NORMAL:
        return SP01_LED_FULL;
    case SP01_POWER_DIAG_LOW:
    case SP01_POWER_DIAG_CRITICAL:
    case SP01_POWER_DIAG_INVALID:
        return SP01_LED_FAULT;
    case SP01_POWER_DIAG_UNKNOWN:
    default:
        return SP01_LED_OFF;
    }
}
