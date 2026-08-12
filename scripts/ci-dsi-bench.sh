#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_root"

make -f Makefile.dsi-bench dsi-bench

test -s build/dsi-bench.elf
test -s build/dsi-bench.map
test -s build/dsi-bench.nds

unit_code=$(od -An -t u1 -j 18 -N 1 build/dsi-bench.nds | tr -d '[:space:]')
if [[ "$unit_code" != "2" && "$unit_code" != "3" ]]; then
    echo "Expected a DSi-capable unit code (2 or 3), got: $unit_code" >&2
    exit 1
fi

echo "DSI_BENCH_UNIT_CODE=$unit_code"
arm_size="${DEVKITARM:-${DEVKITPRO:-/opt/devkitpro}/devkitARM}/bin/arm-none-eabi-size"
"$arm_size" build/dsi-bench.elf
sha256sum build/dsi-bench.nds
