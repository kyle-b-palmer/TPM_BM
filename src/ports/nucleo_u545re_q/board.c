#include "board.h"

#include "stm32u5xx_hal.h"

#include <string.h>

I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef hlpuart1;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_LPUART1_Init(void);

void board_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

void board_console_write(const char *s)
{
    if (s == NULL) {
        return;
    }
    (void)HAL_UART_Transmit(&hlpuart1, (uint8_t *)s, (uint16_t)strlen(s), 1000);
}

void board_init(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_LPUART1_Init();

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_Delay(50);
}

static tpmbm_status board_i2c_write(void *user, uint8_t reg, const uint8_t *data,
                                    size_t len)
{
    uint8_t buf[64];
    HAL_StatusTypeDef hs;
    int tries = 10;
    (void)user;

    if (len + 1u > sizeof(buf)) {
        return TPMBM_ERR_NOMEM;
    }
    buf[0] = reg;
    if (len > 0u && data != NULL) {
        memcpy(buf + 1, data, len);
    }

    do {
        hs = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(TPMBM_I2C_ADDR_7BIT << 1),
                                     buf, (uint16_t)(len + 1u), 250);
        if (hs != HAL_OK) {
            HAL_Delay(1);
        }
    } while (hs != HAL_OK && --tries > 0);

    return (hs == HAL_OK) ? TPMBM_OK : TPMBM_ERR_IO;
}

static tpmbm_status board_i2c_read(void *user, uint8_t reg, uint8_t *data,
                                   size_t len)
{
    HAL_StatusTypeDef hs;
    int tries = 10;
    (void)user;

    if (data == NULL || len == 0u) {
        return TPMBM_ERR_ARG;
    }

    do {
        hs = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(TPMBM_I2C_ADDR_7BIT << 1),
                                     &reg, 1, 250);
        if (hs == HAL_OK) {
            hs = HAL_I2C_Master_Receive(&hi2c1, (uint16_t)(TPMBM_I2C_ADDR_7BIT << 1),
                                        data, (uint16_t)len, 250);
        }
        if (hs != HAL_OK) {
            HAL_Delay(1);
        }
    } while (hs != HAL_OK && --tries > 0);

    return (hs == HAL_OK) ? TPMBM_OK : TPMBM_ERR_IO;
}

static void board_delay_us(void *user, uint32_t us)
{
    uint32_t ms;
    (void)user;
    ms = (us + 999u) / 1000u;
    if (ms == 0u) {
        ms = 1u;
    }
    HAL_Delay(ms);
}

static void board_tpm_reset_pin(void *user, int assert_active_low)
{
    (void)user;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8,
                      assert_active_low ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void board_hal_bind(tpmbm_hal *hal)
{
    hal->user = NULL;
    hal->i2c_write_reg = board_i2c_write;
    hal->i2c_read_reg = board_i2c_read;
    hal->delay_us = board_delay_us;
    hal->tpm_reset = board_tpm_reset_pin;
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        while (1) {
        }
    }

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
    osc.PLL.PLLM = 1;
    osc.PLL.PLLN = 10;
    osc.PLL.PLLP = 2;
    osc.PLL.PLLQ = 2;
    osc.PLL.PLLR = 1;
    osc.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
    osc.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        while (1) {
        }
    }

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 |
                    RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK3;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    clk.APB3CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4) != HAL_OK) {
        while (1) {
        }
    }
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PA8 = TPM_RST_N (active low), start asserted then released in board_init */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
    gpio.Pin = GPIO_PIN_8;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);
}

static void MX_I2C1_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* PB6 SCL, PB7 SDA — AF4 I2C1 */
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &gpio);

    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x00F07BFF; /* ~100-400 kHz class timing; tune on bench */
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        while (1) {
        }
    }
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        while (1) {
        }
    }
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
        while (1) {
        }
    }
}

static void MX_LPUART1_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_LPUART1_CLK_ENABLE();

    /* PA2 TX, PA3 RX — AF8 LPUART1 (ST-LINK VCP) */
    gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = GPIO_AF8_LPUART1;
    HAL_GPIO_Init(GPIOA, &gpio);

    hlpuart1.Instance = LPUART1;
    hlpuart1.Init.BaudRate = 115200;
    hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    hlpuart1.Init.StopBits = UART_STOPBITS_1;
    hlpuart1.Init.Parity = UART_PARITY_NONE;
    hlpuart1.Init.Mode = UART_MODE_TX_RX;
    hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&hlpuart1) != HAL_OK) {
        while (1) {
        }
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void Error_Handler(void)
{
    while (1) {
    }
}
