#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
rectmap_output=$(mktemp "${TMPDIR:-/tmp}/dsi-rectmap-test.XXXXXX")
asset_output=$(mktemp "${TMPDIR:-/tmp}/dsi-asset-test.XXXXXX")
video_output=$(mktemp "${TMPDIR:-/tmp}/dsi-video-test.XXXXXX")
trap 'rm -f "$rectmap_output" "$asset_output" "$video_output"' EXIT

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

# Production renderer: both render paths are compared against an independent
# nearest-neighbour reference, with a padded source pitch and, where the
# toolchain provides them, ASan/UBSan.
build_video_test() {
    g++ -std=c++17 -Wall -Wextra -Werror -g "$@" \
        -I"$repo_root/tests/dsi_video_stubs" \
        -I"$repo_root/src" \
        -I"$repo_root/src/platform/dsi/runtime/include" \
        "$repo_root/src/platform/dsi/runtime/dsi_video.cc" \
        "$repo_root/src/platform/dsi/runtime/dsi_rectmap.cc" \
        "$repo_root/tests/dsi_video_stubs/dsi_runtime_stub.cpp" \
        "$repo_root/tests/dsi_video_test.cpp" \
        -o "$video_output"
}

if ! build_video_test -fsanitize=address,undefined; then
    echo "Sanitizers unavailable; rebuilding the DSi video test without them." >&2
    build_video_test
fi
"$video_output"
