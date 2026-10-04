#ifndef TPMBM_HAL_H
#define TPMBM_HAL_H

#include <stddef.h>
#include <stdint.h>

#include "tpmbm/tpmbm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Board/TPM port callbacks.
 *
 * I2C register access matches the wolfTPM advanced I2C IO shape:
 * write: [reg][payload...], read: write reg then read payload.
 * Default ST33KTPM2X 7-bit address is 0x2E (configured in the port).
 */
typedef struct tpmbm_hal {
    void *user;

    /** Write `len` bytes to TPM I2C register `reg`. */
    tpmbm_status (*i2c_write_reg)(void *user, uint8_t reg, const uint8_t *data,
                                  size_t len);

    /** Read `len` bytes from TPM I2C register `reg`. */
    tpmbm_status (*i2c_read_reg)(void *user, uint8_t reg, uint8_t *data,
                                 size_t len);

    /** Busy-wait or tick delay in microseconds. */
    void (*delay_us)(void *user, uint32_t us);

    /**
     * Drive TPM RESET (active low). NULL if reset is strapped high only.
     * assert_active_low != 0 holds the TPM in reset.
     */
    void (*tpm_reset)(void *user, int assert_active_low);
} tpmbm_hal;

#ifdef __cplusplus
}
#endif

#endif /* TPMBM_HAL_H */
