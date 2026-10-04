#include "host_sim_hal.h"

#include "tpmbm/st33ktpm2x.h"
#include "tpmbm/tpmbm.h"

#include <stdio.h>
#include <string.h>

static void print_digest(const char *label, const uint8_t *d)
{
    size_t i;
    printf("%s: ", label);
    for (i = 0; i < TPMBM_SHA256_DIGEST_SIZE; i++) {
        printf("%02x", d[i]);
    }
    printf("\n");
}

int main(void)
{
    uint8_t sim_storage[4096];
    uint8_t ctx_storage[4096];
    host_sim_tpm *sim = (host_sim_tpm *)sim_storage;
    tpmbm_ctx *ctx = (tpmbm_ctx *)ctx_storage;
    tpmbm_hal hal;
    tpmbm_info info;
    tpmbm_pcr_sha256 pcr;
    tpmbm_status st;
    uint8_t dig[TPMBM_SHA256_DIGEST_SIZE];
    size_t i;

    if (host_sim_tpm_size() > sizeof(sim_storage) ||
        tpmbm_ctx_size() > sizeof(ctx_storage)) {
        fprintf(stderr, "storage too small\n");
        return 1;
    }

    host_sim_init(sim);
    host_sim_hal_bind(&hal, sim);

    st = tpmbm_init(ctx, &hal);
    if (st != TPMBM_OK) {
        fprintf(stderr, "init failed: %s\n", tpmbm_status_str(st));
        return 1;
    }

    st = tpmbm_probe(ctx, &info);
    if (st != TPMBM_OK) {
        fprintf(stderr, "probe failed: %s\n", tpmbm_status_str(st));
        return 1;
    }

    printf("TPM Bare Metal Tools — host_sim\n");
    printf("DID/VID: 0x%04X / 0x%04X  RID: 0x%02X\n", info.device_id,
           info.vendor_id, info.revision);
    printf("Manufacturer: %s (0x%08X)  FW: 0x%08X\n", info.manufacturer_str,
           info.manufacturer, info.firmware_version);
    if (!st33ktpm2x_vendor_matches(info.vendor_id)) {
        printf("warning: vendor_id is not ST (0x%04X)\n", ST33KTPM2X_VENDOR_ID);
    }

    st = tpmbm_pcr_read_sha256(ctx, 0, &pcr);
    if (st != TPMBM_OK) {
        fprintf(stderr, "pcr_read failed: %s\n", tpmbm_status_str(st));
        return 1;
    }
    print_digest("PCR0 before", pcr.digest);

    for (i = 0; i < sizeof(dig); i++) {
        dig[i] = (uint8_t)(0xA0u + i);
    }
    st = tpmbm_pcr_extend_sha256(ctx, 0, dig);
    if (st != TPMBM_OK) {
        fprintf(stderr, "pcr_extend failed: %s\n", tpmbm_status_str(st));
        return 1;
    }

    st = tpmbm_pcr_read_sha256(ctx, 0, &pcr);
    if (st != TPMBM_OK) {
        fprintf(stderr, "pcr_read2 failed: %s\n", tpmbm_status_str(st));
        return 1;
    }
    print_digest("PCR0 after ", pcr.digest);

    tpmbm_shutdown(ctx);
    printf("host_sim OK\n");
    return 0;
}
