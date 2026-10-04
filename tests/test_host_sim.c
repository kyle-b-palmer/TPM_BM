#include "host_sim_hal.h"

#include "tpmbm/st33ktpm2x.h"
#include "tpmbm/tpmbm.h"

#include <stdio.h>
#include <string.h>

static int g_failures;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        g_failures++;
    }
}

int main(void)
{
    uint8_t sim_storage[4096];
    uint8_t ctx_storage[4096];
    host_sim_tpm *sim = (host_sim_tpm *)sim_storage;
    tpmbm_ctx *ctx = (tpmbm_ctx *)ctx_storage;
    tpmbm_hal hal;
    tpmbm_info info;
    tpmbm_pcr_sha256 before, after;
    uint8_t dig[TPMBM_SHA256_DIGEST_SIZE];
    tpmbm_status st;
    size_t i;

    host_sim_init(sim);
    host_sim_hal_bind(&hal, sim);

    st = tpmbm_init(ctx, &hal);
    expect(st == TPMBM_OK, "tpmbm_init");

    st = tpmbm_probe(ctx, &info);
    expect(st == TPMBM_OK, "tpmbm_probe");
    expect(info.vendor_id == ST33KTPM2X_VENDOR_ID, "ST vendor id");
    expect(info.manufacturer == 0x53544D20u, "manufacturer STM ");

    st = tpmbm_pcr_read_sha256(ctx, 0, &before);
    expect(st == TPMBM_OK, "pcr_read before");

    for (i = 0; i < sizeof(dig); i++) {
        dig[i] = (uint8_t)i;
    }
    st = tpmbm_pcr_extend_sha256(ctx, 0, dig);
    expect(st == TPMBM_OK, "pcr_extend");

    st = tpmbm_pcr_read_sha256(ctx, 0, &after);
    expect(st == TPMBM_OK, "pcr_read after");
    expect(memcmp(before.digest, after.digest, TPMBM_SHA256_DIGEST_SIZE) != 0,
           "pcr changed after extend");

    tpmbm_shutdown(ctx);

    if (g_failures == 0) {
        printf("All host_sim tests passed\n");
        return 0;
    }
    fprintf(stderr, "%d failure(s)\n", g_failures);
    return 1;
}
