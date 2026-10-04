# TPM Bare Metal Tools

Portable MCU TPM **host** for the **ST33KTPM2X** over **I2C TIS**, first brought up on the **STM32 Nucleo-U545RE-Q**.

This is **not** a port of Linux `tpm2-tools`. The MCU owns the I2C bus and talks TIS registers directly through a slim SAPI-class command layer. IO callbacks match the wolfTPM advanced I2C shape so wolfTPM can be swapped in later (GPLv2-or-later; see plan).

## Hardware (locked)

| Item | Value |
|------|--------|
| TPM | ST33KTPM2X |
| Board | [NUCLEO-U545RE-Q](https://www.st.com/en/evaluation-tools/nucleo-u545re-q.html) |
| Bus | I2C1 on Arduino header |
| Address | `0x2E` (7-bit) |

### Wiring assumptions

| Function | Nucleo | MCU | Header |
|----------|--------|-----|--------|
| I2C1 SCL | D15 | PB6 | CN5 pin 10 |
| I2C1 SDA | D14 | PB7 | CN5 pin 9 |
| TPM_RST_N | D7 | PA8 | CN9 pin 1 |
| 3V3 | 3V3 | — | CN6 pin 4 |
| GND | GND | — | CN5 pin 7 |

**ST33KTPM2X:** VPS→3V3, GND→GND, SCL→D15, SDA→D14, RST→D7 (active-low, external ~10k pull-up), `GPI_I2C_Select`→GND via ~10k (I2C boot mode). External ~4.7k pull-ups on SDA/SCL to 3V3 (TPM has no internal pull-ups). Console is LPUART1 on PA2/PA3 via ST-LINK VCP at 115200 8N1.

## Repo layout

```
include/tpmbm/     Portable API + ST33 helpers
src/core/          probe / GetCapability / PCR read+extend
src/tis/           I2C TIS (locality, FIFO, DID/VID)
src/ports/host_sim CI-friendly mock TPM
src/ports/nucleo_u545re_q  Board HAL (I2C1 + RST + VCP)
```

## Build (host simulator — no hardware)

```bash
cmake -S . -B build-host -G Ninja -DTPMBM_BUILD_FIRMWARE=OFF
cmake --build build-host
ctest --test-dir build-host --output-on-failure
./build-host/tpmbm_host_sim
```

## Build firmware (Nucleo-U545RE-Q)

Requires `arm-none-eabi-gcc`, `cmake`, `ninja`, and ST CMSIS/HAL (fetched by script):

```bash
./scripts/fetch-deps.sh
cmake -S . -B build-fw -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake \
  -DTPMBM_BUILD_FIRMWARE=ON -DTPMBM_BUILD_HOST=OFF
cmake --build build-fw
```

Artifacts: `build-fw/tpmbm_nucleo_u545re_q.{elf,bin,hex}`.

### Flash

With ST-LINK and `STM32_Programmer_CLI` or OpenOCD / CubeIDE:

```bash
STM32_Programmer_CLI -c port=SWD -w build-fw/tpmbm_nucleo_u545re_q.bin 0x08000000 -v -rst
```

Or open the ELF in STM32CubeIDE / VS Code Cortex-Debug and flash via the on-board ST-LINK.

### Run on hardware

1. Wire ST33 as above; confirm I2C mode select and pull-ups.
2. Flash firmware; open the ST-LINK VCP serial port at 115200.
3. Expect DID/VID, manufacturer via GetCapability, PCR0 read, extend, read-back.

**Honest limit:** this environment cannot exercise a real ST33. Use `host_sim` for CI; validate I2C/TIS on the bench.

## v1 verbs

- `tpmbm_probe` / info (DID/VID/RID + manufacturer)
- `tpmbm_startup_clear`
- `tpmbm_get_capability_uint32`
- `tpmbm_pcr_read_sha256` / `tpmbm_pcr_extend_sha256`

## Docs

Project plan and wiring live in the Project store:

- `docs/st33-nucleo-portable-host.md`
- `docs/project-context.md`

## License notes

Application code in this repo is provided for the TPM Bare Metal Tools project.  
`third_party/wolfTPM` (optional fetch) is wolfSSL **GPLv2-or-later** (commercial license available from wolfSSL). ST CMSIS/HAL retain ST terms.
