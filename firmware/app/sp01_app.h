#ifndef SP01_APP_H
#define SP01_APP_H

#include "app_types.h"
#include "sp01_touch_button.h"

/*
 * SP01 宠物饮水机产品状态机。
 * 业务层只描述产品行为，不直接操作 GPIO/ADC/定时器寄存器。
 */

typedef enum {
    SP01_STATE_BOOT = 0,
    SP01_STATE_STANDBY,
    SP01_STATE_SENSOR_RUN,
    SP01_STATE_MANUAL_RUN,
    SP01_STATE_LOCKOUT,
    SP01_STATE_LOW_WATER_SUSPECT,
    SP01_STATE_LOW_WATER_ALARM,
    SP01_STATE_FAULT
} sp01_state_t;

typedef enum {
    SP01_LED_OFF = 0,
    SP01_LED_STANDBY,
    SP01_LED_PUMPING,
    SP01_LED_CHARGING,
    SP01_LED_FULL,
    SP01_LED_LOW_WATER,
    SP01_LED_FAULT,
    /* 整机测试模式使用的单颗 RGB 灯状态。 */
    SP01_LED_SENSOR_READY,
    SP01_LED_SENSOR_PUMPING,
    SP01_LED_CONTINUOUS,
    /* 通道诊断名按 MCU 丝印命名，不预设 LED 的实际颜色。 */
    SP01_LED_TEST_PB3,
    SP01_LED_TEST_PA5,
    SP01_LED_TEST_PA1,
    /* PA5 隔离测试：其它 RGB 引脚切为高阻，只观察 PA5 的两种电平。 */
    SP01_LED_TEST_ISOLATED_PA5_LOW,
    SP01_LED_TEST_ISOLATED_PA5_HIGH,
    /* PB3 隔离测试：其它 RGB 引脚切为高阻，只观察 PB3 的两种电平。 */
    SP01_LED_TEST_ISOLATED_PB3_LOW,
    SP01_LED_TEST_ISOLATED_PB3_HIGH,
    /* 复现旧版触摸出水灯效：PA5 和 PA1 同时点亮。 */
    SP01_LED_TEST_COMBO_PA5_PA1,
    /* RGB 三路物理电平扫描使用的其它组合。 */
    SP01_LED_TEST_COMBO_PB3_PA5,
    SP01_LED_TEST_COMBO_PB3_PA1,
    SP01_LED_TEST_COMBO_ALL,
    /* 白平衡测试六档（红/绿/蓝占空比由板级软件 PWM 映射）。 */
    SP01_LED_TEST_WHITE_BALANCE_1,
    SP01_LED_TEST_WHITE_BALANCE_2,
    SP01_LED_TEST_WHITE_BALANCE_3,
    SP01_LED_TEST_WHITE_BALANCE_4,
    SP01_LED_TEST_WHITE_BALANCE_5,
    SP01_LED_TEST_WHITE_BALANCE_6,
    /* 规格书产品灯效，和 FullTest001 的测试灯色保持隔离。 */
    SP01_LED_PRODUCT_CONTINUOUS,
    SP01_LED_PRODUCT_CHARGING,
    SP01_LED_PRODUCT_FULL,
    SP01_LED_PRODUCT_LOW_BATTERY,
    SP01_LED_PRODUCT_LOW_WATER_WARNING_TYPEC,
    SP01_LED_PRODUCT_LOW_WATER_WARNING_BATTERY
} sp01_led_mode_t;

typedef enum {
    SP01_BUZZER_OFF = 0,
    SP01_BUZZER_KEY_CLICK,
    SP01_BUZZER_RADAR_TEST,
    SP01_BUZZER_LOW_WATER,
    SP01_BUZZER_FAULT,
    SP01_BUZZER_POWER_ON,
    SP01_BUZZER_POWER_OFF,
    SP01_BUZZER_DOUBLE_CLICK,
    SP01_BUZZER_NIGHT,
    SP01_BUZZER_ERROR
} sp01_buzzer_mode_t;

typedef enum {
    SP01_PUMP_DIAG_UNKNOWN = 0,
    SP01_PUMP_DIAG_NORMAL,
    SP01_PUMP_DIAG_LOW_WATER_SUSPECT,
    SP01_PUMP_DIAG_NO_LOAD,
    SP01_PUMP_DIAG_OVER_CURRENT
} sp01_pump_diag_t;

typedef enum {
    SP01_FAULT_NONE = 0,
    SP01_FAULT_BATTERY_LOW,
    SP01_FAULT_PUMP_OVER_CURRENT,
    SP01_FAULT_LOW_WATER,
    SP01_FAULT_INVALID_STATE
} sp01_fault_code_t;

typedef struct {
    app_bool_t radar_active;
    app_bool_t key_pressed;
    app_bool_t key_long_pressed;
    app_bool_t charging_active;
    app_bool_t charge_done;
    app_bool_t battery_low;
    sp01_pump_diag_t pump_diag;
} sp01_inputs_t;

typedef struct {
    app_bool_t pump_enable;
    app_bool_t radar_power_enable;
    sp01_led_mode_t led_mode;
    sp01_buzzer_mode_t buzzer_mode;
    /* 0 表示使用板级默认频率；非 0 时用于 PWM 载波。 */
    app_u16 buzzer_frequency_hz;
    /* 0 表示使用板级默认占空比；非 0 时为 PWM 高电平占空比百分比。 */
    app_u8 buzzer_duty_percent;
} sp01_outputs_t;

typedef struct {
    sp01_state_t state;
    app_u32 state_elapsed_ms;
    app_u32 lockout_remaining_ms;
    app_u8 low_water_confirm_count;
    app_bool_t night_mode_enabled;
    app_bool_t radar_armed;
    app_bool_t fault_ack_armed;
    sp01_touch_button_t key_button;
    sp01_fault_code_t fault_code;
} sp01_app_t;

void sp01_app_init(sp01_app_t *app);
sp01_outputs_t sp01_app_tick(sp01_app_t *app, const sp01_inputs_t *inputs);
const char *sp01_state_name(sp01_state_t state);
const char *sp01_pump_diag_name(sp01_pump_diag_t diag);
const char *sp01_fault_name(sp01_fault_code_t fault_code);

#endif
