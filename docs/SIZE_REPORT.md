# SIZE_REPORT — Phase 0 Run 1, ARM9 CI pending

## Result class

```text
LOWER BOUND — numeric ARM9 result pending CI/local devkitPro
```

No `.text/.data/.bss` value is invented.

## What changed

The prior CI harness had no build target. This is now fixed.

The repository patch provides:

```text
Makefile.dsi
src/platform/dsi/feasibility/include/SDL.h
src/platform/dsi/feasibility/include/SDL2/SDL.h
src/platform/dsi/feasibility/sdl_feasibility_stubs.cc
```

Command:

```bash
make -f Makefile.dsi feasibility-all V=1
```

Expected outputs when link succeeds:

```text
build/fallout1-dsi-feasibility-baseline.elf
build/fallout1-dsi-feasibility-baseline.map
build/fallout1-dsi-feasibility.elf
build/fallout1-dsi-feasibility.map
```

Baseline uses `-Os -fno-rtti -fno-exceptions` without per-function/data
sections or `--gc-sections`. Optimized adds `-ffunction-sections`,
`-fdata-sections` and `--gc-sections`. Neither uses LTO. Both target
`-march=armv5te -mtune=arm946e-s` and use `ds_arm9.specs`.

The host-only closure link was 789,508 bytes text, 244,976 bytes data and
1,764,392 bytes bss. It is not an ARM9 result, is affected by the host ABI and
libraries, and is excluded from the measured table.

Then:

```bash
arm-none-eabi-size build/fallout1-dsi-feasibility.elf
arm-none-eabi-size -A build/fallout1-dsi-feasibility.elf
```

## Why the first number remains LOWER BOUND

The target compiles the real Fallout core but substitutes a minimal SDL-shaped shim and does not yet link production DSi:

```text
graphics / rectmap renderer
input backend
audio backend
filesystem/SD backend
benchmark/logging runtime
production platform callbacks
ARM7-side application support
libnds/calico runtime attributable to the application
```

No defensible per-subsystem size allowance exists before the first ARM9 map and
native benchmark skeleton are available. Allowances therefore remain
`unknown`; they are not silently added to the lower bound.

`--gc-sections` can also discard code that a production callback graph would keep alive.

Therefore the first successful ELF is a code/static lower bound, not final resident RAM.

## If the first build fails

Do not weaken the target to a trivial subset.

Return:

```text
build/ci/build.log
build/ci/sdl-symbols.txt
build/ci/sdl-call-sites.txt
```

Fix the first real ARM9 compile/link blockers until a meaningful core link is obtained.
