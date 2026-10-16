/*
 * 复制本文件为 vendor_port.c，再按普冉 SDK 改写。
 * 不要把这个模板直接加入量产目标：它没有真实寄存器操作。
 */

#include "board_port_vendor.h"

void board_vendor_init(void)
{
    /* TODO: 打开 GPIO/ADC/定时器时钟，并先把所有执行器配置为关闭。 */
}

void board_vendor_gpio_write(board_vendor_pin_t pin, app_bool_t level)
{
    /* TODO: 将 pin 映射到 PA/PB/PC 的 GPIO 写寄存器或官方 HAL。 */
    (void)pin;
    (void)level;
}

app_bool_t board_vendor_gpio_read(board_vendor_pin_t pin)
{
    /* TODO: 读取 GPIO 输入寄存器并统一返回 APP_FALSE/APP_TRUE。 */
    (void)pin;
    return APP_FALSE;
}

app_u16 board_vendor_adc_read_mv(board_vendor_adc_t channel)
{
    /* TODO: 读取 ADC 原始码，并按实际 VREF/分压换算成 mV。 */
    (void)channel;
    return 0U;
}

app_bool_t board_vendor_take_10ms_tick(void)
{
    /* TODO: 由 10ms 定时器中断置位，再在这里消费一次 tick。 */
    return APP_FALSE;
}
