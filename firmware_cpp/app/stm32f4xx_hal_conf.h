#ifndef __STM32F4xx_HAL_CONF_H
#define __STM32F4xx_HAL_CONF_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f4xx.h"

#define HAL_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED
#define HAL_PCD_MODULE_ENABLED

#define HSE_VALUE    ((uint32_t)8000000U)
#define HSI_VALUE    ((uint32_t)16000000U)
#define HSE_STARTUP_TIMEOUT    ((uint32_t)100U)
#define HSI_STARTUP_TIMEOUT    ((uint32_t)5000U)
#define LSE_STARTUP_TIMEOUT    ((uint32_t)5000U)

#define VDD_VALUE                    3300U
#define TICK_INT_PRIORITY            ((uint32_t)(1U<<__NVIC_PRIO_BITS) - 1U)
#define USE_RTOS                     0U
#define PREFETCH_ENABLE              1U
#define INSTRUCTION_CACHE_ENABLE     1U
#define DATA_CACHE_ENABLE            1U

#define assert_param(expr) ((void)0U)

#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32f4xx_hal_rcc.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32f4xx_hal_gpio.h"
#endif

#ifdef HAL_CORTEX_MODULE_ENABLED
  #include "stm32f4xx_hal_cortex.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32f4xx_hal_flash.h"
#endif

#ifdef HAL_PWR_MODULE_ENABLED
  #include "stm32f4xx_hal_pwr.h"
#endif

#ifdef HAL_DMA_MODULE_ENABLED
  #include "stm32f4xx_hal_dma.h"
#endif

#ifdef HAL_ADC_MODULE_ENABLED
  #include "stm32f4xx_hal_adc.h"
#endif

#ifdef HAL_PCD_MODULE_ENABLED
  #include "stm32f4xx_hal_pcd.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_HAL_CONF_H */
