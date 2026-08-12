#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
rectmap_output=$(mktemp "${TMPDIR:-/tmp}/dsi-rectmap-test.XXXXXX")
asset_output=$(mktemp "${TMPDIR:-/tmp}/dsi-asset-test.XXXXXX")
trap 'rm -f "$rectmap_output" "$asset_output"' EXIT

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$repo_root/src/platform/dsi/bench" \
    "$repo_root/src/platform/dsi/bench/rectmap.cpp" \
    "$repo_root/tests/dsi_rectmap_test.cpp" \
    -o "$rectmap_output"
"$rectmap_output"

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$repo_root/src/platform/dsi/bench" \
    "$repo_root/src/platform/dsi/bench/asset_dat.cpp" \
    "$repo_root/tests/dsi_asset_dat_test.cpp" \
    -o "$asset_output"
"$asset_output"

g++ -std=c++17 -Wall -Wextra -Werror -fsyntax-only \
    -I"$repo_root/tests/dsi_stubs" \
    -I"$repo_root/src/platform/dsi/bench" \
    "$repo_root/src/platform/dsi/bench/asset_probe.cpp"
echo "DSi asset probe host syntax check: PASS"
