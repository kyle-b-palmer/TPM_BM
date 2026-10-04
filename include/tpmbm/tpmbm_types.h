#ifndef TPMBM_TYPES_H
#define TPMBM_TYPES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tpmbm_status {
    TPMBM_OK = 0,
    TPMBM_ERR_ARG = -1,
    TPMBM_ERR_IO = -2,
    TPMBM_ERR_TIMEOUT = -3,
    TPMBM_ERR_PROTOCOL = -4,
    TPMBM_ERR_TPM = -5,
    TPMBM_ERR_NOMEM = -6,
    TPMBM_ERR_UNSUPPORTED = -7,
    TPMBM_ERR_STATE = -8
} tpmbm_status;

#ifndef TPMBM_SHA256_DIGEST_SIZE
#define TPMBM_SHA256_DIGEST_SIZE 32u
#endif

#ifndef TPMBM_MAX_RESPONSE
#define TPMBM_MAX_RESPONSE 1024u
#endif

#ifndef TPMBM_MAX_PCR
#define TPMBM_MAX_PCR 24u
#endif

typedef struct tpmbm_info {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t revision;
    uint8_t locality;
    uint32_t manufacturer;
    char manufacturer_str[8];
    uint32_t firmware_version;
} tpmbm_info;

typedef struct tpmbm_pcr_sha256 {
    uint32_t index;
    uint8_t digest[TPMBM_SHA256_DIGEST_SIZE];
} tpmbm_pcr_sha256;

#ifdef __cplusplus
}
#endif

#endif /* TPMBM_TYPES_H */
