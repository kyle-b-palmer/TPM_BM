#ifndef TPMBM_HOST_SIM_HAL_H
#define TPMBM_HOST_SIM_HAL_H

#include <stddef.h>

#include "tpmbm/tpmbm_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct host_sim_tpm host_sim_tpm;

void host_sim_init(host_sim_tpm *tpm);
void host_sim_hal_bind(tpmbm_hal *hal, host_sim_tpm *tpm);
size_t host_sim_tpm_size(void);

#ifdef __cplusplus
}
#endif

#endif
