#!/usr/bin/env bash
# Local/container equivalent of .github/workflows/dsi-size.yml.
# Run from repository root.
set +e
set -o pipefail

CI_OUT="${CI_OUT:-build/ci}"
EXPECTED_ELF="${EXPECTED_ELF:-build/fallout1-dsi-feasibility.elf}"
EXPECTED_MAP="${EXPECTED_MAP:-build/fallout1-dsi-feasibility.map}"
DEVKITPRO_DIR="${DEVKITPRO:-/opt/devkitpro}"

mkdir -p "${CI_OUT}"

if [[ ! -d "${DEVKITPRO_DIR}/libnds" ]] || ! command -v ndstool >/dev/null 2>&1; then
  echo "libnds and/or ndstool missing; attempting nds-dev install" | tee -a "${CI_OUT}/bootstrap.log"
  if command -v dkp-pacman >/dev/null 2>&1; then
    dkp-pacman -S --needed --noconfirm nds-dev 2>&1 | tee -a "${CI_OUT}/bootstrap.log"
  else
    echo "dkp-pacman not found" | tee -a "${CI_OUT}/bootstrap.log"
  fi
fi

DS_PORTLIBS="${PORTLIBS:-${DEVKITPRO_DIR}/portlibs/nds}"
if [[ ! -f "${DS_PORTLIBS}/lib/libz.a" ]]; then
  if command -v dkp-pacman >/dev/null 2>&1; then
    dkp-pacman -S --needed --noconfirm nds-zlib 2>&1 | tee -a "${CI_OUT}/bootstrap.log" || true
  fi
fi

{
  echo "=== git ==="
  git rev-parse HEAD || true
  echo
  echo "=== environment ==="
  echo "DEVKITPRO=${DEVKITPRO:-UNSET}"
  echo "DEVKITARM=${DEVKITARM:-UNSET}"
  echo "LIBNDS=${LIBNDS:-UNSET}"
  echo "PORTLIBS=${PORTLIBS:-UNSET}"
  echo
  echo "=== tools ==="
  command -v arm-none-eabi-g++ || true
  arm-none-eabi-g++ --version || true
  command -v arm-none-eabi-size || true
  arm-none-eabi-size --version || true
  command -v ndstool || true
  ndstool -h 2>&1 | head -n 30 || true
  echo
  echo "=== libnds ==="
  ls -la "${DEVKITPRO_DIR}/libnds" 2>&1 || true
} | tee "${CI_OUT}/environment.txt"

if [[ -d src ]]; then
  grep -RhoE \
    --include='*.c' --include='*.cc' --include='*.cpp' \
    --include='*.h' --include='*.hpp' \
    'SDL_[A-Za-z0-9_]+' src \
    | sort -u > "${CI_OUT}/sdl-symbols.txt" || true

  grep -RInE \
    --include='*.c' --include='*.cc' --include='*.cpp' \
    --include='*.h' --include='*.hpp' \
    '(^|[^A-Za-z0-9_])SDL_[A-Za-z0-9_]+' src \
    > "${CI_OUT}/sdl-call-sites.txt" || true
fi

make -f Makefile.dsi feasibility-all V=1 2>&1 | tee "${CI_OUT}/build.log"
build_status=${PIPESTATUS[0]}
echo "${build_status}" > "${CI_OUT}/build-exit-code.txt"

elf=""
if [[ -f "${EXPECTED_ELF}" ]]; then
  elf="${EXPECTED_ELF}"
else
  elf="$(find build . -maxdepth 6 -type f -name '*.elf' 2>/dev/null | head -n 1)"
fi

{
  echo "RESULT_CLASS=LOWER_BOUND"
  echo "EXPECTED_ELF=${EXPECTED_ELF}"
  echo "EXPECTED_MAP=${EXPECTED_MAP}"
  echo "FOUND_ELF=${elf:-NONE}"
  echo
  if [[ -n "${elf}" && -f "${elf}" ]]; then
    echo "=== arm-none-eabi-size ==="
    arm-none-eabi-size "${elf}" || true
    echo
    echo "=== arm-none-eabi-size -A ==="
    arm-none-eabi-size -A "${elf}" || true
  else
    echo "No ELF was produced. See build.log."
  fi
} | tee "${CI_OUT}/size.txt"

mkdir -p "${CI_OUT}/maps"
find . -maxdepth 6 -type f -name '*.map' -print > "${CI_OUT}/maps-found.txt" 2>/dev/null || true
while IFS= read -r mapfile; do
  [[ -z "${mapfile}" ]] && continue
  cp -f "${mapfile}" "${CI_OUT}/maps/$(basename "${mapfile}")" || true
done < "${CI_OUT}/maps-found.txt"

if [[ -n "${elf}" && -f "${elf}" ]]; then
  cp -f "${elf}" "${CI_OUT}/$(basename "${elf}")" || true
fi

exit "${build_status}"
