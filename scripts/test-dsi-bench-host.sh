#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
output=$(mktemp "${TMPDIR:-/tmp}/dsi-rectmap-test.XXXXXX")
trap 'rm -f "$output"' EXIT

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$repo_root/src/platform/dsi/bench" \
    "$repo_root/src/platform/dsi/bench/rectmap.cpp" \
    "$repo_root/tests/dsi_rectmap_test.cpp" \
    -o "$output"
"$output"
