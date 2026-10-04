#include "tpmbm/tpmbm.h"

#include "tpmbm/st33ktpm2x.h"
#include "tpmbm/tpmbm_tis.h"

#include <string.h>

/* From tpmbm_cmd.c */
void tpmbm_put_u16(uint8_t *p, uint16_t v);
void tpmbm_put_u32(uint8_t *p, uint32_t v);
uint16_t tpmbm_get_u16(const uint8_t *p);
uint32_t tpmbm_get_u32(const uint8_t *p);
size_t tpmbm_cmd_header(uint8_t *buf, size_t cap, uint32_t cc, size_t body_len);
tpmbm_status tpmbm_check_response(const uint8_t *resp, size_t resp_len,
                                  uint32_t *tpm_rc);

struct tpmbm_ctx {
    tpmbm_hal hal;
    tpmbm_tis tis;
    int ready;
    uint8_t cmd[512];
    uint8_t resp[TPMBM_MAX_RESPONSE];
};

size_t tpmbm_ctx_size(void)
{
    return sizeof(tpmbm_ctx);
}

const char *tpmbm_status_str(tpmbm_status st)
{
    switch (st) {
    case TPMBM_OK:
        return "ok";
    case TPMBM_ERR_ARG:
        return "invalid argument";
    case TPMBM_ERR_IO:
        return "I2C/IO failure";
    case TPMBM_ERR_TIMEOUT:
        return "timeout";
    case TPMBM_ERR_PROTOCOL:
        return "TIS/protocol error";
    case TPMBM_ERR_TPM:
        return "TPM returned an error code";
    case TPMBM_ERR_NOMEM:
        return "buffer too small";
    case TPMBM_ERR_UNSUPPORTED:
        return "unsupported";
    case TPMBM_ERR_STATE:
        return "bad state";
    default:
        return "unknown";
    }
}

static void manuf_to_str(uint32_t manuf, char out[8])
{
    out[0] = (char)((manuf >> 24) & 0xFFu);
    out[1] = (char)((manuf >> 16) & 0xFFu);
    out[2] = (char)((manuf >> 8) & 0xFFu);
    out[3] = (char)(manuf & 0xFFu);
    out[4] = '\0';
    out[5] = '\0';
    out[6] = '\0';
    out[7] = '\0';
}

tpmbm_status tpmbm_init(tpmbm_ctx *ctx, const tpmbm_hal *hal)
{
    tpmbm_status st;

    if (ctx == NULL || hal == NULL) {
        return TPMBM_ERR_ARG;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->hal = *hal;
    st = tpmbm_tis_init(&ctx->tis, &ctx->hal);
    if (st != TPMBM_OK) {
        return st;
    }
    st = tpmbm_tis_request_locality(&ctx->tis, 2000u);
    if (st != TPMBM_OK) {
        return st;
    }
    st = tpmbm_tis_read_info(&ctx->tis);
    if (st != TPMBM_OK) {
        return st;
    }
    ctx->ready = 1;
    return TPMBM_OK;
}

void tpmbm_shutdown(tpmbm_ctx *ctx)
{
    if (ctx == NULL) {
        return;
    }
    if (ctx->ready) {
        (void)tpmbm_tis_release_locality(&ctx->tis);
    }
    memset(ctx, 0, sizeof(*ctx));
}

tpmbm_status tpmbm_hardware_reset(tpmbm_ctx *ctx)
{
    if (ctx == NULL || !ctx->ready) {
        return TPMBM_ERR_STATE;
    }
    if (ctx->hal.tpm_reset == NULL) {
        return TPMBM_ERR_UNSUPPORTED;
    }
    ctx->hal.tpm_reset(ctx->hal.user, 1);
    if (ctx->hal.delay_us != NULL) {
        ctx->hal.delay_us(ctx->hal.user, 10000u);
    }
    ctx->hal.tpm_reset(ctx->hal.user, 0);
    if (ctx->hal.delay_us != NULL) {
        ctx->hal.delay_us(ctx->hal.user, 50000u);
    }
    return tpmbm_tis_request_locality(&ctx->tis, 2000u);
}

tpmbm_status tpmbm_startup_clear(tpmbm_ctx *ctx)
{
    size_t cmd_len;
    size_t resp_len = 0u;
    tpmbm_status st;
    uint32_t tpm_rc = 0u;

    if (ctx == NULL || !ctx->ready) {
        return TPMBM_ERR_STATE;
    }

    cmd_len = tpmbm_cmd_header(ctx->cmd, sizeof(ctx->cmd), TPMBM_TPM_CC_Startup,
                               2u);
    if (cmd_len == 0u) {
        return TPMBM_ERR_NOMEM;
    }
    tpmbm_put_u16(ctx->cmd + 10, TPMBM_TPM_SU_CLEAR);

    st = tpmbm_tis_send_command(&ctx->tis, ctx->cmd, cmd_len, ctx->resp,
                                sizeof(ctx->resp), &resp_len);
    if (st != TPMBM_OK) {
        return st;
    }
    st = tpmbm_check_response(ctx->resp, resp_len, &tpm_rc);
    /* 0x100 = TPM_RC_INITIALIZE: already started — treat as OK for bring-up. */
    if (st == TPMBM_ERR_TPM && tpm_rc == 0x00000100u) {
        return TPMBM_OK;
    }
    return st;
}

tpmbm_status tpmbm_get_capability_uint32(tpmbm_ctx *ctx, uint32_t property,
                                         uint32_t *value)
{
    size_t cmd_len;
    size_t resp_len = 0u;
    tpmbm_status st;
    uint32_t tpm_rc = 0u;

    if (ctx == NULL || !ctx->ready || value == NULL) {
        return TPMBM_ERR_ARG;
    }

    /* body: capability(4) + property(4) + propertyCount(4) */
    cmd_len = tpmbm_cmd_header(ctx->cmd, sizeof(ctx->cmd),
                               TPMBM_TPM_CC_GetCapability, 12u);
    if (cmd_len == 0u) {
        return TPMBM_ERR_NOMEM;
    }
    tpmbm_put_u32(ctx->cmd + 10, TPMBM_TPM_CAP_TPM_PROPERTIES);
    tpmbm_put_u32(ctx->cmd + 14, property);
    tpmbm_put_u32(ctx->cmd + 18, 1u);

    st = tpmbm_tis_send_command(&ctx->tis, ctx->cmd, cmd_len, ctx->resp,
                                sizeof(ctx->resp), &resp_len);
    if (st != TPMBM_OK) {
        return st;
    }
    st = tpmbm_check_response(ctx->resp, resp_len, &tpm_rc);
    if (st != TPMBM_OK) {
        return st;
    }

    /* Response: hdr(10) + moreData(1) + capability(4) + TPML_TAGGED_TPM_PROPERTY:
       count(4) + TPM_PT(4) + value(4)  => need at least 27 bytes */
    if (resp_len < 27u) {
        return TPMBM_ERR_PROTOCOL;
    }
    *value = tpmbm_get_u32(ctx->resp + 23);
    return TPMBM_OK;
}

tpmbm_status tpmbm_probe(tpmbm_ctx *ctx, tpmbm_info *out)
{
    tpmbm_status st;
    uint32_t manuf = 0u;
    uint32_t fw = 0u;

    if (ctx == NULL || !ctx->ready || out == NULL) {
        return TPMBM_ERR_ARG;
    }

    memset(out, 0, sizeof(*out));
    out->vendor_id = ctx->tis.vid;
    out->device_id = ctx->tis.did;
    out->revision = ctx->tis.rid;
    out->locality = ctx->tis.locality;

    (void)tpmbm_startup_clear(ctx);

    st = tpmbm_get_capability_uint32(ctx, TPMBM_TPM_PT_MANUFACTURER, &manuf);
    if (st == TPMBM_OK) {
        out->manufacturer = manuf;
        manuf_to_str(manuf, out->manufacturer_str);
    }

    st = tpmbm_get_capability_uint32(ctx, TPMBM_TPM_PT_FIRMWARE_VERSION_1, &fw);
    if (st == TPMBM_OK) {
        out->firmware_version = fw;
    }

    /* Probe succeeds if TIS identity was read; capability is best-effort. */
    (void)st33ktpm2x_vendor_matches(out->vendor_id);
    return TPMBM_OK;
}

tpmbm_status tpmbm_pcr_read_sha256(tpmbm_ctx *ctx, uint32_t pcr_index,
                                   tpmbm_pcr_sha256 *out)
{
    size_t cmd_len;
    size_t resp_len = 0u;
    tpmbm_status st;
    uint32_t tpm_rc = 0u;
    uint8_t pcr_select[3];
    size_t off;

    if (ctx == NULL || !ctx->ready || out == NULL ||
        pcr_index >= TPMBM_MAX_PCR) {
        return TPMBM_ERR_ARG;
    }

    memset(pcr_select, 0, sizeof(pcr_select));
    pcr_select[pcr_index / 8u] = (uint8_t)(1u << (pcr_index % 8u));

    /* TPML_PCR_SELECTION count=1, hash=SHA256, sizeOfSelect=3, pcrSelect */
    cmd_len = tpmbm_cmd_header(ctx->cmd, sizeof(ctx->cmd), TPMBM_TPM_CC_PCR_Read,
                               4u + 2u + 1u + 3u);
    if (cmd_len == 0u) {
        return TPMBM_ERR_NOMEM;
    }
    off = 10u;
    tpmbm_put_u32(ctx->cmd + off, 1u);
    off += 4u;
    tpmbm_put_u16(ctx->cmd + off, TPMBM_TPM_ALG_SHA256);
    off += 2u;
    ctx->cmd[off++] = 3u;
    memcpy(ctx->cmd + off, pcr_select, 3u);

    st = tpmbm_tis_send_command(&ctx->tis, ctx->cmd, cmd_len, ctx->resp,
                                sizeof(ctx->resp), &resp_len);
    if (st != TPMBM_OK) {
        return st;
    }
    st = tpmbm_check_response(ctx->resp, resp_len, &tpm_rc);
    if (st != TPMBM_OK) {
        return st;
    }

    /*
     * Response layout (simplified parse):
     * hdr(10) + pcrUpdateCounter(4) + TPML_PCR_SELECTION + TPML_DIGEST
     * We search for a 32-byte digest after the selection echo.
     */
    if (resp_len < 10u + 4u + 4u + 2u + 1u + 3u + 4u + 2u + 32u) {
        return TPMBM_ERR_PROTOCOL;
    }

    /* After selection: count(4) of digests, then size(2)=32, then digest */
    off = 10u + 4u;                   /* skip update counter */
    off += 4u + 2u + 1u + 3u;         /* skip echoed TPML_PCR_SELECTION (count=1) */
    if (off + 4u + 2u + 32u > resp_len) {
        return TPMBM_ERR_PROTOCOL;
    }
    if (tpmbm_get_u32(ctx->resp + off) < 1u) {
        return TPMBM_ERR_PROTOCOL;
    }
    off += 4u;
    if (tpmbm_get_u16(ctx->resp + off) != TPMBM_SHA256_DIGEST_SIZE) {
        return TPMBM_ERR_PROTOCOL;
    }
    off += 2u;

    out->index = pcr_index;
    memcpy(out->digest, ctx->resp + off, TPMBM_SHA256_DIGEST_SIZE);
    return TPMBM_OK;
}

tpmbm_status tpmbm_pcr_extend_sha256(
    tpmbm_ctx *ctx, uint32_t pcr_index,
    const uint8_t digest[TPMBM_SHA256_DIGEST_SIZE])
{
    size_t cmd_len;
    size_t resp_len = 0u;
    size_t off;
    tpmbm_status st;
    uint32_t tpm_rc = 0u;

    if (ctx == NULL || !ctx->ready || digest == NULL ||
        pcr_index >= TPMBM_MAX_PCR) {
        return TPMBM_ERR_ARG;
    }

    /*
     * PCR_Extend:
     * pcrHandle(4) + auth area (password empty session) + TPML_DIGEST_VALUES
     *
     * For bring-up we use a single empty auth session:
     * authorizationSize(4)=9, sessionHandle=TPM_RS_PW(0x40000009),
     * nonce size0, attrs=0, hmac size0.
     */
    cmd_len = tpmbm_cmd_header(ctx->cmd, sizeof(ctx->cmd),
                               TPMBM_TPM_CC_PCR_Extend,
                               4u + 4u + 9u + 4u + 2u + 2u + 32u);
    if (cmd_len == 0u) {
        return TPMBM_ERR_NOMEM;
    }

    off = 10u;
    tpmbm_put_u32(ctx->cmd + off, TPMBM_RH_PCR0 + pcr_index);
    off += 4u;
    tpmbm_put_u32(ctx->cmd + off, 9u); /* authorizationSize */
    off += 4u;
    tpmbm_put_u32(ctx->cmd + off, 0x40000009u); /* TPM_RS_PW */
    off += 4u;
    ctx->cmd[off++] = 0u; /* nonce size */
    ctx->cmd[off++] = 0u; /* session attributes */
    ctx->cmd[off++] = 0u; /* hmac size */

    tpmbm_put_u32(ctx->cmd + off, 1u); /* digests count */
    off += 4u;
    tpmbm_put_u16(ctx->cmd + off, TPMBM_TPM_ALG_SHA256);
    off += 2u;
    tpmbm_put_u16(ctx->cmd + off, TPMBM_SHA256_DIGEST_SIZE);
    off += 2u;
    memcpy(ctx->cmd + off, digest, TPMBM_SHA256_DIGEST_SIZE);

    /* Fix total size — cmd_header already set based on body_len. */
    st = tpmbm_tis_send_command(&ctx->tis, ctx->cmd, cmd_len, ctx->resp,
                                sizeof(ctx->resp), &resp_len);
    if (st != TPMBM_OK) {
        return st;
    }
    return tpmbm_check_response(ctx->resp, resp_len, &tpm_rc);
}
