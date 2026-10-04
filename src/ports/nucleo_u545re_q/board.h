#ifndef TPMBM_BOARD_NUCLEO_U545RE_Q_H
#define TPMBM_BOARD_NUCLEO_U545RE_Q_H

#include "tpmbm/tpmbm_hal.h"
#include "tpmbm/st33ktpm2x.h"

/*
 * Assumed wiring (Arduino Uno V3 headers on NUCLEO-U545RE-Q / MB1841):
 *
 *   Function        Nucleo signal   MCU pin   Header
 *   -------------   -------------   -------   ----------------
 *   I2C1_SCL        D15 / I2C_SCL   PB6       CN5 pin 10
 *   I2C1_SDA        D14 / I2C_SDA   PB7       CN5 pin 9
 *   TPM_RST_N       D7              PA8       CN9 pin 1
 *   3V3             IOREF/3V3       —         CN6 pin 4 (3V3)
 *   GND             GND             —         CN5 pin 7 / CN6 pin 6-7
 *
 * ST33KTPM2X side:
 *   VPS  -> 3V3
 *   GND  -> GND
 *   SCL  -> D15 (PB6)
 *   SDA  -> D14 (PB7)
 *   RST  -> D7  (PA8), active-low; add external pull-up (~10k to 3V3)
 *   GPI_I2C_Select -> GND via pull-down (~10k) so the part boots in I2C mode
 *   SDA/SCL external pull-ups (~4.7k to 3V3) required (TPM open-drain, no PU)
 *
 * I2C instance: I2C1 @ 400 kHz, 7-bit address 0x2E
 * Console: LPUART1 on PA2/PA3 via ST-LINK VCP
 */

#ifndef TPMBM_I2C_ADDR_7BIT
#define TPMBM_I2C_ADDR_7BIT ST33KTPM2X_I2C_ADDR_7BIT
#endif

void board_init(void);
void board_hal_bind(tpmbm_hal *hal);
void board_console_write(const char *s);
void board_delay_ms(uint32_t ms);

#endif
