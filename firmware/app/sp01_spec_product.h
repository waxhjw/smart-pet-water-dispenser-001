#ifndef SP01_SPEC_PRODUCT_H
#define SP01_SPEC_PRODUCT_H

#include "sp01_app.h"
#include "sp01_power_monitor.h"

typedef enum {
    SP01_SPEC_SUPPLY_UNKNOWN = 0,
    SP01_SPEC_SUPPLY_BATTERY,
    SP01_SPEC_SUPPLY_TYPEC
} sp01_spec_supply_t;

typedef enum {
    SP01_SPEC_STATE_BOOT = 0,
    SP01_SPEC_STATE_OFF,
    SP01_SPEC_STATE_BATTERY_READY,
    SP01_SPEC_STATE_SENSOR_RUN,
    SP01_SPEC_STATE_SENSOR_LOCKOUT,
    SP01_SPEC_STATE_FUR_FLUSH,
    SP01_SPEC_STATE_TYPEC_CONTINUOUS,
    SP01_SPEC_STATE_LOW_WATER_LOCKOUT
} sp01_spec_state_t;

typedef struct {
    sp01_spec_supply_t supply;
    app_bool_t key_pressed;
    app_bool_t radar_active;
    app_bool_t radar_rising_edge;
    app_bool_t charging_active;
    app_bool_t charge_done;
    app_u16 pump_sense_mv;
    sp01_power_diag_t battery_diag;
} sp01_spec_product_inputs_t;

typedef struct {
    sp01_spec_supply_t supply;
    sp01_spec_supply_t supply_candidate;
    app_u16 supply_candidate_ticks;
    sp01_spec_state_t state;
    sp01_touch_button_t key_button;
    app_bool_t powered_on;
    app_bool_t night_mode;
    app_bool_t radar_armed;
    app_bool_t fur_flush_due;
    app_u32 boot_elapsed_ms;
    app_u32 state_elapsed_ms;
    app_u32 lockout_remaining_ms;
    app_u32 fur_flush_elapsed_ms;
    app_u32 radar_warmup_elapsed_ms;
    app_u32 radar_idle_elapsed_ms;
    app_u32 charge_pulse_elapsed_ms;
    app_u32 pump_run_elapsed_ms;
    app_u32 low_water_led_elapsed_ms;
    app_u32 pump_adc_sum_mv;
    app_u16 pump_adc_sample_count;
    app_u16 pump_adc_last_average_mv;
    app_u8 low_water_consecutive_count;
    /* 007 将“低水位预警”和“停泵保护”拆成两套连续窗口计数。 */
    app_u8 low_water_warning_consecutive_count;
    app_u8 low_water_protection_consecutive_count;
    app_bool_t low_water_warning_active;
    app_bool_t low_water_retry_active;
    app_u32 low_water_lockout_elapsed_ms;
    app_u32 low_water_retry_run_elapsed_ms;
    app_u32 low_water_retry_adc_sum_mv;
    app_u16 low_water_retry_adc_sample_count;
    app_u8 low_water_recovery_high_count;
    app_u8 low_water_retry_schedule_index;
    app_u8 low_water_retry_count;
    sp01_fault_code_t fault_code;
} sp01_spec_product_t;

/* 仅在充电状态没有冲突时，用 CHRG/DONE 与 VBAT 估算当前供电来源。 */
sp01_spec_supply_t sp01_spec_detect_supply(app_bool_t charging_active,
                                           app_bool_t charge_done,
                                           app_bool_t charge_conflict,
                                           app_u16 battery_mv);
void sp01_spec_product_init(sp01_spec_product_t *product);
sp01_outputs_t sp01_spec_product_tick(sp01_spec_product_t *product,
                                      const sp01_spec_product_inputs_t *inputs);
const char *sp01_spec_state_name(sp01_spec_state_t state);

#endif
