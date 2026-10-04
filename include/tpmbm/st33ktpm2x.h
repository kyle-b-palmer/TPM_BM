#ifndef TPMBM_ST33KTPM2X_H
#define TPMBM_ST33KTPM2X_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Default 7-bit I2C address (TCG PTP). */
#define ST33KTPM2X_I2C_ADDR_7BIT 0x2Eu

/** STMicroelectronics PCI SIG vendor ID commonly reported in DID_VID. */
#define ST33KTPM2X_VENDOR_ID 0x104Au

/**
 * Board wiring notes (see docs + README):
 * - VPS: 1.8 V or 3.3 V (Nucleo bring-up uses 3V3)
 * - GPI_I2C_Select: pull-down so the part boots in I2C mode
 * - RST: active-low, must not float; MCU GPIO + external pull-up recommended
 * - SDA/SCL: open-drain, external pull-ups required (TPM has none)
 */

static inline int st33ktpm2x_vendor_matches(uint16_t vendor_id)
{
    return vendor_id == ST33KTPM2X_VENDOR_ID;
}

#ifdef __cplusplus
}
#endif

#endif /* TPMBM_ST33KTPM2X_H */
