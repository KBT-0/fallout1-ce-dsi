# FEASIBILITY — Phase 0 Run 1, ARM9 CI pending

## Current status

```text
BLOCKED — run budget NOT consumed
```

No GO / CONDITIONAL GO / NO-GO decision is justified yet.

The local environment has no devkitARM or configured melonDS. The checked-out
source is present, and all 111 feasibility translation units compile and link
with the host compiler and shim. This is a static closure check only, not an
ARM9 measurement. GitHub Actions is the next authoritative compiler/linker
environment.

## Source/reference evidence already established

Pinned revisions:

```text
Fallout 1 CE upstream:
0609bcfd0ec40ff0571d0f57fab2821eb461dc8b

MrHuu 3DS fork:
fdb9cd7eca0d03191019c410f084196b6e1df972
```

The maintained 3DS Makefile compiles the Fallout core directories plus `src/platform/ctr`, uses GNU++17, disables RTTI/exceptions, and links SDL2/Citro2D/Citro3D/libctru. The DSi feasibility Makefile mirrors the core source-directory set but replaces the CTR platform directory with a Phase-0 shim.

The official current devkitPro ARM9 template uses:

```text
-march=armv5te -mtune=arm946e-s
-specs=ds_arm9.specs
-lnds9
```

The new `Makefile.dsi` follows that target model and does not copy 3DS ARMv6K/hard-float flags.

## Phase 0A status

The CI path now has something real to measure:

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

Both successful linked ELFs remain:

```text
LOWER BOUND
```

because the production DSi graphics/input/audio/filesystem backends are not linked yet and SDL is represented by a minimal compile/link shim.

A failed CI build is still useful: `build/ci/build.log` becomes the first real ARMv5TE compiler/linker blocker list.

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

### Priority 3 — Phase 0A: ARM9 code/static lower bound via CI

Run in parallel/background through GitHub Actions or local devkitPro:

```text
arm-none-eabi-size
arm-none-eabi-size -A
linker map
source-wide SDL inventory
```

Do not block all active 0C/0D design work waiting for this result, but ingest it as soon as CI returns.

### Priority 4 — complete SDL/platform closure

Use CI `sdl-symbols.txt` and `sdl-call-sites.txt` to replace the web audit with an exhaustive checked-out-source inventory and estimate the real production backend cost.

## Remaining authoritative unknowns

```text
ARM9 .text/.rodata/.data/.bss
linker-map largest contributors
real DSi free heap under Unlaunch and TWiLight
representative Fallout runtime working set
real DSi VRAM upload/remap/render timing
real DSi SD throughput/latency
audio CPU/RAM cost
production DSi backend resident cost
```
