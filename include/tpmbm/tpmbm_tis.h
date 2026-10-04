#ifndef TPMBM_TIS_H
#define TPMBM_TIS_H

#include "tpmbm/tpmbm_hal.h"
#include "tpmbm/tpmbm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Locality-0 I2C TIS register map (TCG PTP / wolfTPM I2C layout). */
#define TPMBM_TIS_ACCESS 0x00u
#define TPMBM_TIS_INT_ENABLE 0x08u
#define TPMBM_TIS_INT_VECTOR 0x0Cu
#define TPMBM_TIS_INT_STATUS 0x10u
#define TPMBM_TIS_INTF_CAPS 0x14u
#define TPMBM_TIS_STS 0x18u
#define TPMBM_TIS_DATA_FIFO 0x24u
#define TPMBM_TIS_INTERFACE_ID 0x30u
#define TPMBM_TIS_DID_VID 0x48u
#define TPMBM_TIS_RID 0x4Cu

#define TPMBM_TIS_ACCESS_VALID 0x80u
#define TPMBM_TIS_ACCESS_ACTIVE 0x20u
#define TPMBM_TIS_ACCESS_PENDING 0x04u
#define TPMBM_TIS_ACCESS_REQUEST 0x02u
#define TPMBM_TIS_ACCESS_ESTABLISHMENT 0x01u

#define TPMBM_TIS_STS_VALID 0x80u
#define TPMBM_TIS_STS_COMMAND_READY 0x40u
#define TPMBM_TIS_STS_GO 0x20u
#define TPMBM_TIS_STS_DATA_AVAIL 0x10u
#define TPMBM_TIS_STS_DATA_EXPECT 0x08u

#define TPMBM_TPM_ST_NO_SESSIONS 0x8001u
#define TPMBM_TPM_RC_SUCCESS 0x00000000u

#define TPMBM_TPM_CC_Startup 0x00000144u
#define TPMBM_TPM_CC_SelfTest 0x00000143u
#define TPMBM_TPM_CC_GetCapability 0x0000017Au
#define TPMBM_TPM_CC_PCR_Read 0x0000017Eu
#define TPMBM_TPM_CC_PCR_Extend 0x00000182u

#define TPMBM_TPM_SU_CLEAR 0x0000u
#define TPMBM_TPM_CAP_TPM_PROPERTIES 0x00000006u
#define TPMBM_TPM_PT_MANUFACTURER 0x00000105u
#define TPMBM_TPM_PT_FIRMWARE_VERSION_1 0x0000010Bu
#define TPMBM_TPM_ALG_SHA256 0x000Bu
#define TPMBM_RH_PCR0 0x00000000u

typedef struct tpmbm_tis {
    const tpmbm_hal *hal;
    uint8_t locality;
    uint16_t did;
    uint16_t vid;
    uint8_t rid;
} tpmbm_tis;

tpmbm_status tpmbm_tis_init(tpmbm_tis *tis, const tpmbm_hal *hal);
tpmbm_status tpmbm_tis_request_locality(tpmbm_tis *tis, uint32_t timeout_ms);
tpmbm_status tpmbm_tis_release_locality(tpmbm_tis *tis);
tpmbm_status tpmbm_tis_read_info(tpmbm_tis *tis);
tpmbm_status tpmbm_tis_wait_ready(tpmbm_tis *tis, uint32_t timeout_ms);
tpmbm_status tpmbm_tis_send_command(tpmbm_tis *tis, const uint8_t *cmd,
                                    size_t cmd_len, uint8_t *resp,
                                    size_t resp_cap, size_t *resp_len);

#ifdef __cplusplus
}
#endif

#endif /* TPMBM_TIS_H */
