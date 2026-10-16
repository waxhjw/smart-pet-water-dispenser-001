#include "py32f0xx_hal.h"

#ifdef SP01_RGB_WHITE_BALANCE_TEST
#include "board_port_vendor.h"
#endif
#ifdef SP01_SPEC_TEST007
#include "board_port_vendor.h"
#endif
#ifdef SP01_RGB_TUNER_TEST
#include "board_port_vendor.h"
#endif

void SysTick_Handler(void)
{
    HAL_IncTick();
#ifdef SP01_RGB_WHITE_BALANCE_TEST
    board_vendor_rgb_pwm_tick_1ms();
#endif
}

#ifdef SP01_SPEC_TEST007
void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
    board_vendor_rgb_pwm_tim1_irq();
}
#endif

#ifdef SP01_RGB_TUNER_TEST
void TIM14_IRQHandler(void)
{
    board_vendor_rgb_tuner_tim14_irq();
}
#endif
