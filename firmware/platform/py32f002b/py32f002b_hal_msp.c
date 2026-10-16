#include "py32f0xx_hal.h"

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    GPIO_InitTypeDef gpio = {0};

    if (hadc == 0 || hadc->Instance != ADC1) {
        return;
    }

    __HAL_RCC_ADC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* ADC_XQ 已按硬件变更接到 PA3；PY32F002B 的 PA3 仍是 ADC_IN1。 */
    gpio.Pin = GPIO_PIN_3 | GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_1;
    HAL_GPIO_Init(GPIOB, &gpio);
}

static void board_vendor_tim14_msp_init(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef gpio = {0};

    if (htim == 0 || htim->Instance != TIM14) {
        return;
    }

    __HAL_RCC_TIM14_CLK_ENABLE();
#ifdef SP01_RGB_TUNER_TEST
    /* 调色固件只用 TIM14 更新事件，保持 PB7 为普通 GPIO 低电平。 */
    return;
#endif
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PB7 的官方复用表：AF5 = TIM14_CH1。 */
    gpio.Pin = GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = GPIO_AF5_TIM14;
    HAL_GPIO_Init(GPIOB, &gpio);
}

static void board_vendor_tim1_msp_init(TIM_HandleTypeDef *htim)
{
    if (htim == 0 || htim->Instance != TIM1) {
        return;
    }

    __HAL_RCC_TIM1_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0U, 0U);
    HAL_NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    board_vendor_tim14_msp_init(htim);
    board_vendor_tim1_msp_init(htim);
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    board_vendor_tim14_msp_init(htim);
}
