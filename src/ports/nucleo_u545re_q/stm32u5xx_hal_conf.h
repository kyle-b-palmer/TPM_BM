#ifndef STM32U5XX_HAL_CONF_H
#define STM32U5XX_HAL_CONF_H

#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_ICACHE_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

#define USE_HAL_I2C_REGISTER_CALLBACKS 0U
#define USE_HAL_UART_REGISTER_CALLBACKS 0U

#include "stm32u5xx_hal_rcc.h"
#include "stm32u5xx_hal_gpio.h"
#include "stm32u5xx_hal_dma.h"
#include "stm32u5xx_hal_cortex.h"
#include "stm32u5xx_hal_flash.h"
#include "stm32u5xx_hal_i2c.h"
#include "stm32u5xx_hal_icache.h"
#include "stm32u5xx_hal_pwr.h"
#include "stm32u5xx_hal_uart.h"

#define HSE_VALUE 16000000U
#define HSE_STARTUP_TIMEOUT 100U
#define HSI_VALUE 16000000U
#define HSI_CALIBRATION_VALUE 16U
#define MSI_VALUE 4000000U
#define HSI48_VALUE 48000000U
#define LSE_VALUE 32768U
#define LSE_STARTUP_TIMEOUT 5000U
#define LSI_VALUE 32000U
#define EXTERNAL_SAI1_CLOCK_VALUE 48000U
#define EXTERNAL_SAI2_CLOCK_VALUE 48000U
#define VDD_VALUE 3300U
#define TICK_INT_PRIORITY 15U
#define USE_RTOS 0U
#define PREFETCH_ENABLE 1U

#ifdef USE_FULL_ASSERT
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif

#endif
