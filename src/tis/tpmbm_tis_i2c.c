#include "tpmbm/tpmbm_tis.h"

#include <string.h>

static void delay_ms(const tpmbm_hal *hal, uint32_t ms)
{
    if (hal->delay_us == NULL) {
        return;
    }
    while (ms-- > 0u) {
        hal->delay_us(hal->user, 1000u);
    }
}

static tpmbm_status tis_read(tpmbm_tis *tis, uint8_t reg, uint8_t *data,
                             size_t len)
{
    if (tis == NULL || tis->hal == NULL || tis->hal->i2c_read_reg == NULL) {
        return TPMBM_ERR_ARG;
    }
    return tis->hal->i2c_read_reg(tis->hal->user, reg, data, len);
}

static tpmbm_status tis_write(tpmbm_tis *tis, uint8_t reg, const uint8_t *data,
                              size_t len)
{
    if (tis == NULL || tis->hal == NULL || tis->hal->i2c_write_reg == NULL) {
        return TPMBM_ERR_ARG;
    }
    return tis->hal->i2c_write_reg(tis->hal->user, reg, data, len);
}

static tpmbm_status tis_read_sts(tpmbm_tis *tis, uint8_t *sts)
{
    return tis_read(tis, TPMBM_TIS_STS, sts, 1u);
}

static tpmbm_status tis_write_sts(tpmbm_tis *tis, uint8_t sts)
{
    return tis_write(tis, TPMBM_TIS_STS, &sts, 1u);
}

static tpmbm_status tis_burst_count(tpmbm_tis *tis, uint16_t *burst)
{
    uint8_t buf[3];
    tpmbm_status st;

    /* STS is 1 byte status + 2 byte burstCount (little-endian) in TIS. */
    st = tis_read(tis, TPMBM_TIS_STS, buf, 3u);
    if (st != TPMBM_OK) {
        return st;
    }
    *burst = (uint16_t)buf[1] | ((uint16_t)buf[2] << 8);
    if (*burst == 0u) {
        *burst = 1u;
    }
    return TPMBM_OK;
}

tpmbm_status tpmbm_tis_init(tpmbm_tis *tis, const tpmbm_hal *hal)
{
    if (tis == NULL || hal == NULL) {
        return TPMBM_ERR_ARG;
    }
    memset(tis, 0, sizeof(*tis));
    tis->hal = hal;
    tis->locality = 0u;
    return TPMBM_OK;
}

tpmbm_status tpmbm_tis_request_locality(tpmbm_tis *tis, uint32_t timeout_ms)
{
    uint8_t access = TPMBM_TIS_ACCESS_REQUEST;
    tpmbm_status st;
    uint32_t waited = 0u;

    if (tis == NULL) {
        return TPMBM_ERR_ARG;
    }

    st = tis_write(tis, TPMBM_TIS_ACCESS, &access, 1u);
    if (st != TPMBM_OK) {
        return st;
    }

    while (waited <= timeout_ms) {
        st = tis_read(tis, TPMBM_TIS_ACCESS, &access, 1u);
        if (st != TPMBM_OK) {
            return st;
        }
        if ((access & TPMBM_TIS_ACCESS_ACTIVE) != 0u &&
            (access & TPMBM_TIS_ACCESS_VALID) != 0u) {
            return TPMBM_OK;
        }
        delay_ms(tis->hal, 1u);
        waited++;
    }
    return TPMBM_ERR_TIMEOUT;
}

tpmbm_status tpmbm_tis_release_locality(tpmbm_tis *tis)
{
    uint8_t access = TPMBM_TIS_ACCESS_ACTIVE;
    if (tis == NULL) {
        return TPMBM_ERR_ARG;
    }
    return tis_write(tis, TPMBM_TIS_ACCESS, &access, 1u);
}

tpmbm_status tpmbm_tis_read_info(tpmbm_tis *tis)
{
    uint8_t did_vid[4];
    uint8_t rid = 0u;
    tpmbm_status st;

    if (tis == NULL) {
        return TPMBM_ERR_ARG;
    }

    st = tis_read(tis, TPMBM_TIS_DID_VID, did_vid, sizeof(did_vid));
    if (st != TPMBM_OK) {
        return st;
    }
    /* Little-endian: VID then DID */
    tis->vid = (uint16_t)did_vid[0] | ((uint16_t)did_vid[1] << 8);
    tis->did = (uint16_t)did_vid[2] | ((uint16_t)did_vid[3] << 8);

    st = tis_read(tis, TPMBM_TIS_RID, &rid, 1u);
    if (st != TPMBM_OK) {
        return st;
    }
    tis->rid = rid;
    return TPMBM_OK;
}

tpmbm_status tpmbm_tis_wait_ready(tpmbm_tis *tis, uint32_t timeout_ms)
{
    uint32_t waited = 0u;
    tpmbm_status st;
    uint8_t sts;

    if (tis == NULL) {
        return TPMBM_ERR_ARG;
    }

    st = tis_write_sts(tis, TPMBM_TIS_STS_COMMAND_READY);
    if (st != TPMBM_OK) {
        return st;
    }

    while (waited <= timeout_ms) {
        st = tis_read_sts(tis, &sts);
        if (st != TPMBM_OK) {
            return st;
        }
        if ((sts & TPMBM_TIS_STS_COMMAND_READY) != 0u) {
            return TPMBM_OK;
        }
        delay_ms(tis->hal, 1u);
        waited++;
    }
    return TPMBM_ERR_TIMEOUT;
}

static tpmbm_status tis_wait_status(tpmbm_tis *tis, uint8_t mask, uint8_t value,
                                    uint32_t timeout_ms)
{
    uint32_t waited = 0u;
    while (waited <= timeout_ms) {
        uint8_t sts = 0u;
        tpmbm_status st = tis_read_sts(tis, &sts);
        if (st != TPMBM_OK) {
            return st;
        }
        if ((sts & mask) == value) {
            return TPMBM_OK;
        }
        delay_ms(tis->hal, 1u);
        waited++;
    }
    return TPMBM_ERR_TIMEOUT;
}

tpmbm_status tpmbm_tis_send_command(tpmbm_tis *tis, const uint8_t *cmd,
                                    size_t cmd_len, uint8_t *resp,
                                    size_t resp_cap, size_t *resp_len)
{
    size_t sent = 0u;
    size_t got = 0u;
    uint16_t burst = 0u;
    tpmbm_status st;
    uint32_t rsp_size = 0u;

    if (tis == NULL || cmd == NULL || resp == NULL || resp_len == NULL ||
        cmd_len < 10u || resp_cap < 10u) {
        return TPMBM_ERR_ARG;
    }

    st = tpmbm_tis_wait_ready(tis, 2000u);
    if (st != TPMBM_OK) {
        return st;
    }

    while (sent < cmd_len) {
        size_t chunk;
        st = tis_burst_count(tis, &burst);
        if (st != TPMBM_OK) {
            return st;
        }
        chunk = burst;
        if (chunk > (cmd_len - sent)) {
            chunk = cmd_len - sent;
        }
        st = tis_write(tis, TPMBM_TIS_DATA_FIFO, cmd + sent, chunk);
        if (st != TPMBM_OK) {
            return st;
        }
        sent += chunk;
    }

    st = tis_write_sts(tis, TPMBM_TIS_STS_GO);
    if (st != TPMBM_OK) {
        return st;
    }

    st = tis_wait_status(tis, TPMBM_TIS_STS_DATA_AVAIL | TPMBM_TIS_STS_VALID,
                         TPMBM_TIS_STS_DATA_AVAIL | TPMBM_TIS_STS_VALID, 5000u);
    if (st != TPMBM_OK) {
        return st;
    }

    /* Read header first (tag + size + rc) = 10 bytes */
    while (got < 10u) {
        size_t chunk;
        st = tis_burst_count(tis, &burst);
        if (st != TPMBM_OK) {
            return st;
        }
        chunk = burst;
        if (chunk > (10u - got)) {
            chunk = 10u - got;
        }
        st = tis_read(tis, TPMBM_TIS_DATA_FIFO, resp + got, chunk);
        if (st != TPMBM_OK) {
            return st;
        }
        got += chunk;
    }

    rsp_size = ((uint32_t)resp[2] << 24) | ((uint32_t)resp[3] << 16) |
               ((uint32_t)resp[4] << 8) | (uint32_t)resp[5];
    if (rsp_size < 10u || rsp_size > resp_cap) {
        (void)tis_write_sts(tis, TPMBM_TIS_STS_COMMAND_READY);
        return TPMBM_ERR_PROTOCOL;
    }

    while (got < rsp_size) {
        size_t chunk;
        st = tis_burst_count(tis, &burst);
        if (st != TPMBM_OK) {
            return st;
        }
        chunk = burst;
        if (chunk > (rsp_size - got)) {
            chunk = rsp_size - got;
        }
        st = tis_read(tis, TPMBM_TIS_DATA_FIFO, resp + got, chunk);
        if (st != TPMBM_OK) {
            return st;
        }
        got += chunk;
    }

    *resp_len = rsp_size;
    (void)tis_write_sts(tis, TPMBM_TIS_STS_COMMAND_READY);
    return TPMBM_OK;
}
