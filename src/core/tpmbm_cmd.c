#include "tpmbm/tpmbm_tis.h"

#include <string.h>

/* Internal helpers shared with tpmbm.c via header-less linkage. */
void tpmbm_put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)((v >> 8) & 0xFFu);
    p[1] = (uint8_t)(v & 0xFFu);
}

void tpmbm_put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)((v >> 24) & 0xFFu);
    p[1] = (uint8_t)((v >> 16) & 0xFFu);
    p[2] = (uint8_t)((v >> 8) & 0xFFu);
    p[3] = (uint8_t)(v & 0xFFu);
}

uint16_t tpmbm_get_u16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

uint32_t tpmbm_get_u32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

size_t tpmbm_cmd_header(uint8_t *buf, size_t cap, uint32_t cc, size_t body_len)
{
    size_t total = 10u + body_len;
    if (buf == NULL || cap < total) {
        return 0u;
    }
    tpmbm_put_u16(buf, TPMBM_TPM_ST_NO_SESSIONS);
    tpmbm_put_u32(buf + 2, (uint32_t)total);
    tpmbm_put_u32(buf + 6, cc);
    return total;
}

tpmbm_status tpmbm_check_response(const uint8_t *resp, size_t resp_len,
                                  uint32_t *tpm_rc)
{
    uint32_t size;
    uint32_t rc;

    if (resp == NULL || resp_len < 10u) {
        return TPMBM_ERR_PROTOCOL;
    }
    size = tpmbm_get_u32(resp + 2);
    if (size != resp_len) {
        return TPMBM_ERR_PROTOCOL;
    }
    rc = tpmbm_get_u32(resp + 6);
    if (tpm_rc != NULL) {
        *tpm_rc = rc;
    }
    if (rc != TPMBM_TPM_RC_SUCCESS) {
        /* TPM_RC_INITIALIZE (0x100) after duplicate Startup is tolerable upstream. */
        return TPMBM_ERR_TPM;
    }
    return TPMBM_OK;
}
