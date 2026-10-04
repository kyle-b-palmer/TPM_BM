#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TP="$ROOT/third_party"
mkdir -p "$TP"
clone() {
  local url="$1" dir="$2" branch="${3:-}"
  if [[ -d "$dir/.git" || -f "$dir/CMakeLists.txt" || -d "$dir/Include" || -d "$dir/Inc" || -d "$dir/CMSIS" || -d "$dir/wolftpm" ]]; then
    echo "ok: $dir"
    return 0
  fi
  if [[ -n "$branch" ]]; then
    git clone --depth 1 --branch "$branch" "$url" "$dir"
  else
    git clone --depth 1 "$url" "$dir"
  fi
}
clone https://github.com/STMicroelectronics/cmsis_core.git "$TP/cmsis_core"
clone https://github.com/STMicroelectronics/cmsis_device_u5.git "$TP/cmsis_device_u5"
clone https://github.com/STMicroelectronics/stm32u5xx_hal_driver.git "$TP/stm32u5xx_hal_driver"
clone https://github.com/wolfSSL/wolfTPM.git "$TP/wolfTPM" v3.9.2
echo "Dependencies ready under $TP"
echo "Note: wolfTPM is vendored for reference / future integration (GPLv2-or-later)."
