#ifndef TPMBM_H
#define TPMBM_H

#include "tpmbm/tpmbm_hal.h"
#include "tpmbm/tpmbm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tpmbm_ctx tpmbm_ctx;

/** Opaque context size for static allocation. */
size_t tpmbm_ctx_size(void);

/**
 * Initialize a host context on top of a board HAL.
 * Performs locality request and reads DID/VID when possible.
 */
tpmbm_status tpmbm_init(tpmbm_ctx *ctx, const tpmbm_hal *hal);

/** Release locality / clear context. Safe on NULL. */
void tpmbm_shutdown(tpmbm_ctx *ctx);

/** Pulse TPM reset (if HAL provides it), then re-probe locality. */
tpmbm_status tpmbm_hardware_reset(tpmbm_ctx *ctx);

/**
 * Probe the TPM over I2C TIS: locality, DID/VID/RID, optional GetCapability
 * for manufacturer.
 */
tpmbm_status tpmbm_probe(tpmbm_ctx *ctx, tpmbm_info *out);

/** TPM2_GetCapability(TPM_CAP_TPM_PROPERTIES, property, count). */
tpmbm_status tpmbm_get_capability_uint32(tpmbm_ctx *ctx, uint32_t property,
                                         uint32_t *value);

/** Issue TPM2_Startup(TPM_SU_CLEAR). Idempotent enough for bring-up. */
tpmbm_status tpmbm_startup_clear(tpmbm_ctx *ctx);

/** Read one PCR bank digest (SHA-256) for `pcr_index` (0..23). */
tpmbm_status tpmbm_pcr_read_sha256(tpmbm_ctx *ctx, uint32_t pcr_index,
                                   tpmbm_pcr_sha256 *out);

/**
 * Extend PCR `pcr_index` with a SHA-256 digest (TPM_ALG_SHA256).
 * Digest must be TPMBM_SHA256_DIGEST_SIZE bytes.
 */
tpmbm_status tpmbm_pcr_extend_sha256(tpmbm_ctx *ctx, uint32_t pcr_index,
                                     const uint8_t digest[TPMBM_SHA256_DIGEST_SIZE]);

/** Human-readable status string. */
const char *tpmbm_status_str(tpmbm_status st);

#ifdef __cplusplus
}
#endif

#endif /* TPMBM_H */
