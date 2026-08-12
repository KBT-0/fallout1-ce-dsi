# FEASIBILITY — Phase 0 ARM9 lower bound complete

## Current status

```text
UNKNOWN — Phase 0A passes; Phase 0C/0D evidence pending
```

No GO / CONDITIONAL GO / NO-GO decision is justified yet.

GitHub Actions run `31626178248` compiled all 111 feasibility translation
units, linked baseline and optimized ARM9 ELFs, emitted both linker maps, and
passed output enforcement. The optimized `text + data + bss` lower bound is
2,660,844 bytes; see `SIZE_REPORT.md` for the exact breakdown and omissions.

## Source/reference evidence already established

Pinned revisions:

```text
Fallout 1 CE upstream:
0609bcfd0ec40ff0571d0f57fab2821eb461dc8b

MrHuu 3DS fork:
fdb9cd7eca0d03191019c410f084196b6e1df972
```

The maintained 3DS Makefile compiles the Fallout core directories plus `src/platform/ctr`, uses GNU++17, disables RTTI/exceptions, and links SDL2/Citro2D/Citro3D/libctru. The DSi feasibility Makefile mirrors the core source-directory set but replaces the CTR platform directory with a Phase-0 shim.

The current official devkitPro rules use:

```text
-march=armv5te -mtune=arm946e-s
-specs=${CALICO}/share/ds9.specs
-lnds9 -lcalico_ds9
```

`Makefile.dsi` follows that target model and does not copy 3DS ARMv6K/hard-float flags.

## Phase 0A status

The CI path builds:

```text
make -f Makefile.dsi feasibility-all
```

Expected output contract:

```text
build/fallout1-dsi-feasibility.elf
build/fallout1-dsi-feasibility.map
```

The target now produces both required lower-bound configurations:

```text
build/fallout1-dsi-feasibility-baseline.elf
build/fallout1-dsi-feasibility-baseline.map
build/fallout1-dsi-feasibility.elf
build/fallout1-dsi-feasibility.map
```

Both successful linked ELFs are classified:

```text
LOWER BOUND
```

because the production DSi graphics/input/audio/filesystem backends are not linked yet and SDL is represented by a minimal compile/link shim.

Phase 0A passed after correcting portlibs expansion, isolating library paths
from `ds_rules`, and using the current Calico startup contract.

Phase 0C and 0D designs are now recorded in `RAM_BUDGET.md` and
`RENDER_BENCH.md`. GNW current/peak allocation counters are exposed for the
runtime checkpoint logger; physical-DSi values remain unknown.

## 3DS package pre-signal

```text
INDICATIVE — NOT MEASURED, DIFFERENT ARCH/SDK
```

Previously recorded release metadata:

```text
fallout-ce.3dsx : 2.44 MB
fallout-ce.cia  : 2.52 MB
```

This is not resident RAM and is excluded from GO/NO-GO arithmetic.

### Priority interpretation

The low-single-digit-MB packaged 3DS application makes it less likely that **resident code size alone** is the dominant blocker. That is only an indicative signal, not a conclusion.

Accordingly, active engineering priority is now information-value ordered rather than section-number ordered.

## Required next evidence — priority order

### Priority 1 — Phase 0D: DSi render/VRAM update cost

Highest-value active measurement because it can independently produce a hard performance NO-GO.

Build the unified DSi benchmark and measure on real DSi:

```text
- VRAM bank remap cost
- 8-bit/indexed texture upload cost
- DMA/copy cost
- one-screen 3D textured-quad path
- 2D bitmap/background path
- hybrid: one screen 3D + other screen 2D
- representative dirty-region/chunk updates
- total ms/frame, not emulator FPS
```

melonDS may validate functional behavior only. Real hardware supplies authoritative timing.

### Priority 2 — Phase 0C: representative runtime working set

Determine peak memory for a real gameplay state, especially:

```text
engine baseline
real map
art cache
objects/critters
scripts/text
framebuffer/render staging
I/O/decompression
minimum audio OFF
minimum audio ON
allocation spikes + fragmentation headroom
```

This is expected to be the main RAM GO/NO-GO determinant, not packaged executable size.

### Priority 3 — production backend closure

Replace the feasibility shim incrementally and measure the delta for:

```text
graphics and VRAM staging
input
filesystem/SD
audio off/on
benchmark logging
```

Use the successful CI `sdl-symbols.txt` and `sdl-call-sites.txt` as the checked-out-source inventory. Preserve baseline/optimized map comparisons after each backend lands.

## Remaining authoritative unknowns

```text
real DSi free heap under Unlaunch and TWiLight
representative Fallout runtime working set
real DSi VRAM upload/remap/render timing
real DSi SD throughput/latency
audio CPU/RAM cost
production DSi backend resident cost
```
