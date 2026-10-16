#ifndef PY32F002B_BOARD_PORT_H
#define PY32F002B_BOARD_PORT_H

#include "../../app/app_types.h"
#include "../../app/sp01_app.h"

typedef struct {
    app_bool_t radar_active;
    app_bool_t key_pressed;
    app_bool_t key_long_pressed;
    app_bool_t charging_active;
    app_bool_t charge_done;
    app_u16 pump_sense_mv;
    app_u16 battery_mv;
    app_u16 ntc_mv;
} board_inputs_t;

typedef enum {
    BOARD_RESET_REASON_UNKNOWN = 0,
    BOARD_RESET_REASON_IWDG = 1
} board_reset_reason_t;

void board_init(void);
void board_watchdog_refresh(void);
board_reset_reason_t board_get_reset_reason(void);
void board_safe_outputs_off(void);
void board_apply_outputs(const sp01_outputs_t *outputs);
board_inputs_t board_read_inputs(void);
/* 只读指定 ADC 外部通道，供 SWD 调试扫描使用；不会改变 GPIO 或执行器状态。 */
app_u16 board_adc_read_channel_raw(app_u8 adc_channel);
app_u16 board_adc_read_channel_mv(app_u8 adc_channel);
app_bool_t board_take_10ms_tick(void);
void board_tick_10ms_hook(void);

#ifdef HOST_SIM
void board_sim_set_inputs(board_inputs_t inputs);
sp01_outputs_t board_sim_get_outputs(void);
void board_sim_force_tick(void);
#endif

#endif
