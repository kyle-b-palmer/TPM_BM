#include "board.h"

#include "tpmbm/st33ktpm2x.h"
#include "tpmbm/tpmbm.h"

#include <stdio.h>
#include <string.h>

int _write(int fd, char *ptr, int len)
{
    char tmp[128];
    int off = 0;
    (void)fd;
    if (ptr == NULL || len <= 0) {
        return 0;
    }
    while (off < len) {
        int n = len - off;
        if (n > (int)sizeof(tmp) - 1) {
            n = (int)sizeof(tmp) - 1;
        }
        memcpy(tmp, ptr + off, (size_t)n);
        tmp[n] = '\0';
        board_console_write(tmp);
        off += n;
    }
    return len;
}

static void print_digest(const char *label, const uint8_t *d)
{
    char line[96];
    int n;
    size_t i;
    n = snprintf(line, sizeof(line), "%s: ", label);
    for (i = 0; i < TPMBM_SHA256_DIGEST_SIZE && n > 0 && (size_t)n + 2 < sizeof(line); i++) {
        n += snprintf(line + n, sizeof(line) - (size_t)n, "%02x", d[i]);
    }
    board_console_write(line);
    board_console_write("\r\n");
}

int main(void)
{
    uint8_t ctx_storage[4096];
    tpmbm_ctx *ctx = (tpmbm_ctx *)ctx_storage;
    tpmbm_hal board_hal;

    tpmbm_info info;
    tpmbm_pcr_sha256 pcr;
    tpmbm_status st;
    uint8_t dig[TPMBM_SHA256_DIGEST_SIZE];
    char line[160];
    size_t i;

    board_init();
    board_console_write("\r\nTPM Bare Metal Tools — Nucleo-U545RE-Q / ST33KTPM2X\r\n");

    if (tpmbm_ctx_size() > sizeof(ctx_storage)) {
        board_console_write("ctx storage too small\r\n");
        while (1) {
        }
    }

    board_hal_bind(&board_hal);
    st = tpmbm_init(ctx, &board_hal);
    if (st != TPMBM_OK) {
        snprintf(line, sizeof(line), "tpmbm_init failed: %s\r\n", tpmbm_status_str(st));
        board_console_write(line);
        board_console_write("Check wiring, 0x2E address, I2C pull-ups, and RST.\r\n");
        while (1) {
        }
    }

    st = tpmbm_probe(ctx, &info);
    if (st != TPMBM_OK) {
        snprintf(line, sizeof(line), "tpmbm_probe failed: %s\r\n", tpmbm_status_str(st));
        board_console_write(line);
        while (1) {
        }
    }

    snprintf(line, sizeof(line),
             "DID/VID: 0x%04X / 0x%04X  RID: 0x%02X\r\n",
             info.device_id, info.vendor_id, info.revision);
    board_console_write(line);
    snprintf(line, sizeof(line),
             "Manufacturer: %s (0x%08lX)  FW: 0x%08lX\r\n",
             info.manufacturer_str,
             (unsigned long)info.manufacturer,
             (unsigned long)info.firmware_version);
    board_console_write(line);

    if (!st33ktpm2x_vendor_matches(info.vendor_id)) {
        board_console_write("warning: DID_VID vendor is not ST 0x104A\r\n");
    }

    st = tpmbm_pcr_read_sha256(ctx, 0, &pcr);
    if (st != TPMBM_OK) {
        snprintf(line, sizeof(line), "pcr_read failed: %s\r\n", tpmbm_status_str(st));
        board_console_write(line);
    } else {
        print_digest("PCR0 before", pcr.digest);
    }

    for (i = 0; i < sizeof(dig); i++) {
        dig[i] = (uint8_t)(0xA0u + i);
    }
    st = tpmbm_pcr_extend_sha256(ctx, 0, dig);
    if (st != TPMBM_OK) {
        snprintf(line, sizeof(line), "pcr_extend failed: %s\r\n", tpmbm_status_str(st));
        board_console_write(line);
    } else {
        st = tpmbm_pcr_read_sha256(ctx, 0, &pcr);
        if (st == TPMBM_OK) {
            print_digest("PCR0 after ", pcr.digest);
        }
    }

    board_console_write("bring-up sequence complete\r\n");
    while (1) {
        board_delay_ms(1000);
    }
}
