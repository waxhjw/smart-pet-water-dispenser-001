#ifndef SP01_PY32F002B_HAL_CONF_H
#define SP01_PY32F002B_HAL_CONF_H

/*
 * SP01 只启用实际使用到的官方 HAL 模块。
 * 这样可以避免把无关外设和代码一起拉进 24KB Flash 的目标程序。
 */
#define HAL_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_IWDG_MODULE_ENABLED

#define HSI_VALUE              ((uint32_t)24000000U)
#define HSE_VALUE              ((uint32_t)24000000U)
#define LSI_VALUE              ((uint32_t)32768U)
#define LSE_VALUE              ((uint32_t)32768U)
#define VDD_VALUE              ((uint32_t)3300U)
#define HSE_STARTUP_TIMEOUT    ((uint32_t)200U)
#define LSE_STARTUP_TIMEOUT    ((uint32_t)5000U)

#define PRIORITY_HIGHEST       0U
#define PRIORITY_HIGH          1U
#define PRIORITY_LOW           2U
#define PRIORITY_LOWEST        3U
#define TICK_INT_PRIORITY      ((uint32_t)PRIORITY_LOWEST)
#define USE_RTOS               0U
#define PREFETCH_ENABLE        0U

#include "py32f0xx_hal.h"
#include "py32f002b_hal_rcc.h"
#include "py32f002b_hal_gpio.h"
#include "py32f002b_hal_cortex.h"
#include "py32f002b_hal_adc.h"
#include "py32f002b_hal_tim.h"
#include "py32f002b_hal_iwdg.h"
#include "py32f002b_hal_gpio_ex.h"
#include "py32f002b_hal_flash.h"
#include "py32f002b_hal_pwr.h"

#ifndef assert_param
#define assert_param(expr) ((void)0U)
#endif

#endif
