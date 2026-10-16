#ifndef PY32F002B_BOARD_PORT_VENDOR_H
#define PY32F002B_BOARD_PORT_VENDOR_H

/*
 * 这是 PY32F002B 厂商 SDK 的唯一接入口。
 *
 * 这个文件故意不包含任何普冉寄存器头文件，避免把 SDK 类型扩散到
 * app/ 业务层。拿到 SDK 后，只需要在 vendor_port.c 里把这些函数映射
 * 到 GPIO、ADC、定时器寄存器或官方 HAL。
 */

#include "../../app/app_types.h"

typedef enum {
    BOARD_VENDOR_PIN_PUMP_ON = 0,
    BOARD_VENDOR_PIN_RADAR_POWER,
    BOARD_VENDOR_PIN_RADAR_INPUT,
    BOARD_VENDOR_PIN_KEY,
    BOARD_VENDOR_PIN_CHARGE_ACTIVE,
    BOARD_VENDOR_PIN_CHARGE_DONE,
    BOARD_VENDOR_PIN_LED_RED,
    BOARD_VENDOR_PIN_LED_GREEN,
    BOARD_VENDOR_PIN_LED_BLUE,
    BOARD_VENDOR_PIN_BUZZER
} board_vendor_pin_t;

typedef enum {
    BOARD_VENDOR_ADC_PUMP_SENSE = 0,
    BOARD_VENDOR_ADC_BATTERY,
    BOARD_VENDOR_ADC_NTC
} board_vendor_adc_t;

typedef enum {
    BOARD_VENDOR_RESET_UNKNOWN = 0,
    BOARD_VENDOR_RESET_IWDG = 1
} board_vendor_reset_reason_t;

typedef enum {
    BOARD_VENDOR_RGB_PA5_OFF = 0,
    BOARD_VENDOR_RGB_PA5_LOW,
    BOARD_VENDOR_RGB_PA5_HIGH
} board_vendor_rgb_pa5_state_t;

typedef enum {
    BOARD_VENDOR_RGB_ISOLATED_PA5 = 0,
    BOARD_VENDOR_RGB_ISOLATED_PB3
} board_vendor_rgb_isolated_channel_t;

void board_vendor_init(void);
void board_vendor_watchdog_refresh(void);
board_vendor_reset_reason_t board_vendor_get_reset_reason(void);
void board_vendor_gpio_write(board_vendor_pin_t pin, app_bool_t level);
void board_vendor_rgb_pwm_apply(app_u8 red_duty_percent,
                                app_u8 green_duty_percent,
                                app_u8 blue_duty_percent);
void board_vendor_rgb_pwm_tick_1ms(void);
void board_vendor_rgb_pwm_tim1_irq(void);
void board_vendor_rgb_tuner_tim14_irq(void);
void board_vendor_rgb_tristate_apply(app_bool_t red,
                                     app_bool_t green,
                                     app_bool_t blue,
                                     app_u8 active_level);
void board_vendor_rgb_isolated_apply(board_vendor_rgb_isolated_channel_t channel,
                                     board_vendor_rgb_pa5_state_t state);
void board_vendor_buzzer_apply(app_bool_t enabled,
                               app_u16 frequency_hz,
                               app_u8 duty_percent);
app_bool_t board_vendor_gpio_read(board_vendor_pin_t pin);
app_u16 board_vendor_adc_read_mv(board_vendor_adc_t channel);
/* 读取一个 ADC 通道的 12 位原始码；函数内部会先清空旧通道选择。 */
app_u16 board_vendor_adc_read_channel_raw(app_u8 adc_channel);
/* 只读 ADC 外部通道 0~7；不会改变 GPIO 模式，未知通道需硬件确认。 */
app_u16 board_vendor_adc_read_channel_mv(app_u8 adc_channel);
app_bool_t board_vendor_take_10ms_tick(void);

#endif
