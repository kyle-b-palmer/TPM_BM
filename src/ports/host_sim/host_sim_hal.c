#include "host_sim_hal.h"

#include "tpmbm/st33ktpm2x.h"
#include "tpmbm/tpmbm_tis.h"

#include <string.h>

#define HOST_SIM_FIFO 1024

struct host_sim_tpm {
    uint8_t access;
    uint8_t sts;
    uint16_t burst;
    uint8_t rid;
    uint16_t vid;
    uint16_t did;
    uint8_t cmd[HOST_SIM_FIFO];
    size_t cmd_len;
    size_t cmd_expected;
    uint8_t rsp[HOST_SIM_FIFO];
    size_t rsp_len;
    size_t rsp_off;
    uint8_t pcr[TPMBM_MAX_PCR][TPMBM_SHA256_DIGEST_SIZE];
    int started;
};

size_t host_sim_tpm_size(void)
{
    return sizeof(host_sim_tpm);
}

static uint16_t get_be16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t get_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void put_be16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static void put_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static void sim_extend(uint8_t pcr[32], const uint8_t dig[32])
{
    uint8_t out[32];
    int i;
    for (i = 0; i < 32; i++) {
        out[i] = (uint8_t)(pcr[i] ^ dig[i] ^ (uint8_t)(i * 17));
    }
    memcpy(pcr, out, 32);
}

static void build_rsp_header(uint8_t *rsp, size_t *len, uint32_t rc,
                             const uint8_t *body, size_t body_len)
{
    size_t total = 10u + body_len;
    put_be16(rsp, TPMBM_TPM_ST_NO_SESSIONS);
    put_be32(rsp + 2, (uint32_t)total);
    put_be32(rsp + 6, rc);
    if (body_len > 0u && body != NULL) {
        memcpy(rsp + 10, body, body_len);
    }
    *len = total;
}

static void process_command(host_sim_tpm *t)
{
    uint32_t cc;
    uint8_t body[256];
    size_t body_len = 0u;
    uint32_t rc = TPMBM_TPM_RC_SUCCESS;

    if (t->cmd_len < 10u) {
        build_rsp_header(t->rsp, &t->rsp_len, 0x80001u, NULL, 0u);
        return;
    }
    cc = get_be32(t->cmd + 6);

    if (cc == TPMBM_TPM_CC_Startup) {
        if (t->started) {
            rc = 0x00000100u;
        }
        t->started = 1;
        build_rsp_header(t->rsp, &t->rsp_len, rc, NULL, 0u);
    } else if (cc == TPMBM_TPM_CC_GetCapability) {
        uint32_t prop = get_be32(t->cmd + 14);
        uint32_t value = 0u;
        if (prop == TPMBM_TPM_PT_MANUFACTURER) {
            value = 0x53544D20u;
        } else if (prop == TPMBM_TPM_PT_FIRMWARE_VERSION_1) {
            value = 0x00090001u;
        }
        body[0] = 0;
        put_be32(body + 1, TPMBM_TPM_CAP_TPM_PROPERTIES);
        put_be32(body + 5, 1u);
        put_be32(body + 9, prop);
        put_be32(body + 13, value);
        body_len = 17u;
        build_rsp_header(t->rsp, &t->rsp_len, rc, body, body_len);
    } else if (cc == TPMBM_TPM_CC_PCR_Read) {
        uint8_t sel = t->cmd[17];
        uint32_t idx = 0u;
        while (idx < 8u && ((sel >> idx) & 1u) == 0u) {
            idx++;
        }
        put_be32(body + body_len, 1u);
        body_len += 4u;
        put_be32(body + body_len, 1u);
        body_len += 4u;
        put_be16(body + body_len, TPMBM_TPM_ALG_SHA256);
        body_len += 2u;
        body[body_len++] = 3u;
        body[body_len++] = sel;
        body[body_len++] = 0;
        body[body_len++] = 0;
        put_be32(body + body_len, 1u);
        body_len += 4u;
        put_be16(body + body_len, 32u);
        body_len += 2u;
        memcpy(body + body_len, t->pcr[idx], 32u);
        body_len += 32u;
        build_rsp_header(t->rsp, &t->rsp_len, rc, body, body_len);
    } else if (cc == TPMBM_TPM_CC_PCR_Extend) {
        uint32_t handle = get_be32(t->cmd + 10);
        uint32_t idx = handle & 0xFFu;
        const uint8_t *dig = t->cmd + 10u + 4u + 4u + 9u + 4u + 2u + 2u;
        if (idx < TPMBM_MAX_PCR) {
            sim_extend(t->pcr[idx], dig);
        }
        build_rsp_header(t->rsp, &t->rsp_len, rc, NULL, 0u);
    } else {
        build_rsp_header(t->rsp, &t->rsp_len, 0x0000008Bu, NULL, 0u);
    }

    t->rsp_off = 0u;
    t->sts = TPMBM_TIS_STS_VALID | TPMBM_TIS_STS_DATA_AVAIL;
    t->cmd_len = 0u;
    t->cmd_expected = 0u;
}

void host_sim_init(host_sim_tpm *tpm)
{
    memset(tpm, 0, sizeof(*tpm));
    tpm->access = TPMBM_TIS_ACCESS_VALID | TPMBM_TIS_ACCESS_ESTABLISHMENT;
    tpm->sts = TPMBM_TIS_STS_VALID | TPMBM_TIS_STS_COMMAND_READY;
    tpm->burst = 64u;
    tpm->rid = 0x01u;
    tpm->vid = ST33KTPM2X_VENDOR_ID;
    tpm->did = 0x0010u;
}

static tpmbm_status sim_write(void *user, uint8_t reg, const uint8_t *data,
                              size_t len)
{
    host_sim_tpm *t = (host_sim_tpm *)user;
    if (data == NULL && len > 0u) {
        return TPMBM_ERR_ARG;
    }

    switch (reg) {
    case TPMBM_TIS_ACCESS:
        if (len < 1u) {
            return TPMBM_ERR_ARG;
        }
        if ((data[0] & TPMBM_TIS_ACCESS_REQUEST) != 0u) {
            t->access = TPMBM_TIS_ACCESS_VALID | TPMBM_TIS_ACCESS_ACTIVE;
        }
        if ((data[0] & TPMBM_TIS_ACCESS_ACTIVE) != 0u) {
            t->access = TPMBM_TIS_ACCESS_VALID | TPMBM_TIS_ACCESS_ESTABLISHMENT;
        }
        return TPMBM_OK;
    case TPMBM_TIS_STS:
        if (len < 1u) {
            return TPMBM_ERR_ARG;
        }
        if ((data[0] & TPMBM_TIS_STS_COMMAND_READY) != 0u) {
            t->sts = TPMBM_TIS_STS_VALID | TPMBM_TIS_STS_COMMAND_READY;
            t->cmd_len = 0u;
            t->cmd_expected = 0u;
            t->rsp_len = 0u;
            t->rsp_off = 0u;
        }
        if ((data[0] & TPMBM_TIS_STS_GO) != 0u) {
            process_command(t);
        }
        return TPMBM_OK;
    case TPMBM_TIS_DATA_FIFO:
        if (t->cmd_len + len > sizeof(t->cmd)) {
            return TPMBM_ERR_NOMEM;
        }
        memcpy(t->cmd + t->cmd_len, data, len);
        t->cmd_len += len;
        if (t->cmd_len >= 6u && t->cmd_expected == 0u) {
            t->cmd_expected = get_be32(t->cmd + 2);
        }
        t->sts = TPMBM_TIS_STS_VALID | TPMBM_TIS_STS_DATA_EXPECT;
        if (t->cmd_expected != 0u && t->cmd_len >= t->cmd_expected) {
            t->sts = TPMBM_TIS_STS_VALID;
        }
        return TPMBM_OK;
    default:
        return TPMBM_OK;
    }
}

static tpmbm_status sim_read(void *user, uint8_t reg, uint8_t *data, size_t len)
{
    host_sim_tpm *t = (host_sim_tpm *)user;
    if (data == NULL || len == 0u) {
        return TPMBM_ERR_ARG;
    }
    memset(data, 0, len);

    switch (reg) {
    case TPMBM_TIS_ACCESS:
        data[0] = t->access;
        return TPMBM_OK;
    case TPMBM_TIS_STS:
        data[0] = t->sts;
        if (len >= 3u) {
            data[1] = (uint8_t)(t->burst & 0xFFu);
            data[2] = (uint8_t)((t->burst >> 8) & 0xFFu);
        }
        return TPMBM_OK;
    case TPMBM_TIS_DID_VID:
        if (len < 4u) {
            return TPMBM_ERR_ARG;
        }
        data[0] = (uint8_t)(t->vid & 0xFFu);
        data[1] = (uint8_t)((t->vid >> 8) & 0xFFu);
        data[2] = (uint8_t)(t->did & 0xFFu);
        data[3] = (uint8_t)((t->did >> 8) & 0xFFu);
        return TPMBM_OK;
    case TPMBM_TIS_RID:
        data[0] = t->rid;
        return TPMBM_OK;
    case TPMBM_TIS_DATA_FIFO: {
        size_t n = len;
        if (t->rsp_off >= t->rsp_len) {
            return TPMBM_ERR_PROTOCOL;
        }
        if (n > t->rsp_len - t->rsp_off) {
            n = t->rsp_len - t->rsp_off;
        }
        memcpy(data, t->rsp + t->rsp_off, n);
        t->rsp_off += n;
        if (t->rsp_off >= t->rsp_len) {
            t->sts = TPMBM_TIS_STS_VALID | TPMBM_TIS_STS_COMMAND_READY;
        } else {
            t->sts = TPMBM_TIS_STS_VALID | TPMBM_TIS_STS_DATA_AVAIL;
        }
        return TPMBM_OK;
    }
    default:
        return TPMBM_OK;
    }
}

static void sim_delay(void *user, uint32_t us)
{
    (void)user;
    (void)us;
}

static void sim_reset(void *user, int assert_active_low)
{
    host_sim_tpm *t = (host_sim_tpm *)user;
    if (!assert_active_low) {
        host_sim_init(t);
    }
}

void host_sim_hal_bind(tpmbm_hal *hal, host_sim_tpm *tpm)
{
    hal->user = tpm;
    hal->i2c_write_reg = sim_write;
    hal->i2c_read_reg = sim_read;
    hal->delay_us = sim_delay;
    hal->tpm_reset = sim_reset;
}
