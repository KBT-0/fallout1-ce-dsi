#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_root"

make -f Makefile.dsi dsi-game

test -s build/fallout1-dsi.elf
test -s build/fallout1-dsi.map
test -s build/fallout1-dsi.nds

unit_code=$(od -An -t u1 -j 18 -N 1 build/fallout1-dsi.nds | tr -d '[:space:]')
if [[ "$unit_code" != "2" && "$unit_code" != "3" ]]; then
    echo "Expected a DSi-capable unit code (2 or 3), got: $unit_code" >&2
    exit 1
fi

echo "DSI_GAME_UNIT_CODE=$unit_code"
arm_size="${DEVKITARM:-${DEVKITPRO:-/opt/devkitpro}/devkitARM}/bin/arm-none-eabi-size"
"$arm_size" build/fallout1-dsi.elf
sha256sum build/fallout1-dsi.nds
