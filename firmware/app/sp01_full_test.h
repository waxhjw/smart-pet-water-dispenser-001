#ifndef SP01_FULL_TEST_H
#define SP01_FULL_TEST_H

#include "app_types.h"
#include "sp01_app.h"
#include "sp01_touch_button.h"

/* 仅用于无电池 Type-C 整机验证，不进入产品状态机。 */
typedef enum {
    SP01_FULL_POWER_BOOT = 0,
    SP01_FULL_POWER_OFF,
    SP01_FULL_POWER_ON_SENSOR,
    SP01_FULL_POWER_ON_CONTINUOUS
} sp01_full_power_state_t;

typedef struct {
    app_bool_t key_pressed;
    app_bool_t radar_active;
    app_bool_t radar_rising_edge;
    sp01_pump_diag_t pump_diag;
    /* 仅 BRINGUP/SWD 测试使用：0 无注入，1 低水位，2 过流故障。 */
    app_u8 injected_fault;
} sp01_full_test_inputs_t;

typedef struct {
    sp01_full_power_state_t power_state;
    sp01_touch_button_t key_button;
    app_bool_t night_mode_enabled;
    app_bool_t radar_armed;
    app_bool_t pump_running;
    app_bool_t pump_from_radar;
    app_bool_t pending_short_click;
    app_bool_t low_water_active;
    app_bool_t low_water_beep_pending;
    app_bool_t low_water_beeped;
    app_bool_t fault_active;
    app_u32 state_elapsed_ms;
    app_u32 pump_elapsed_ms;
    app_u32 radar_cooldown_ms;
    app_u32 pending_click_elapsed_ms;
    sp01_fault_code_t fault_code;
    sp01_buzzer_mode_t buzzer_request;
    sp01_pump_diag_t observed_pump_diag;
} sp01_full_test_t;

void sp01_full_test_init(sp01_full_test_t *test);
sp01_outputs_t sp01_full_test_tick(sp01_full_test_t *test,
                                   const sp01_full_test_inputs_t *inputs);
const char *sp01_full_power_state_name(sp01_full_power_state_t state);

#endif
