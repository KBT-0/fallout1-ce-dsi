# CTR_AUDIT — 3DS reference implementation

## Pinned reference

Repository: `MrHuu/fallout1-ce-3ds`
Branch: `3DS`
Commit: `fdb9cd7eca0d03191019c410f084196b6e1df972` (`Update 3DS port`)

## `src/platform/ctr` contents inspected

The current 3DS platform layer contains:

```text
ctr_gfx.cc
ctr_gfx.h
ctr_input.cc
ctr_input.h
ctr_rectmap.cc
ctr_rectmap.h
ctr_sys.cc
ctr_sys.h
vshader.v.pica
```

## Build model

`Makefile.ctr`:
- includes `src/platform/ctr` in the build;
- uses GNU++17;
- disables RTTI and exceptions;
- uses 3DS-specific ARMv6K/mpcore/hard-float flags;
- links SDL2 + Citro2D + Citro3D + libctru + zlib/math;
- already emits a linker map in its 3DS build;
- uses 3DS portlibs for SDL2.

The 3DS CPU/ABI flags are explicitly reference-only and must not be copied to the DSi target. Current devkitPro DS templates target ARM9 using `-march=armv5te -mtune=arm946e-s`.

## Graphics architecture observed

The 3DS backend is not a simple SDL renderer wrapper. It contains a dedicated native graphics path.

Observed characteristics:
- top and bottom Citro3D render targets;
- a dedicated render texture sized 1024x512;
- rectmap-driven drawing for different Fallout UI states;
- conversion/copy from an SDL surface into the native 3DS rendering path;
- debug overlays for FPS/memory.

The current 3DS implementation therefore provides two useful references:
1. the **UX/rectmap architecture**, which should be preserved conceptually;
2. a caution that the renderer is deeply hardware-specific and must be replaced for DSi.

## Input architecture observed

The 3DS backend combines SDL event handling with native 3DS HID input.

Key behaviors include:
- touch coordinate mapping through active rectmaps into Fallout source coordinates;
- native button/analog reading;
- `R` mapped to left mouse;
- `L` mapped to right mouse;
- `A` used for field/GUI display switching where applicable;
- `SELECT`-based zoom/display behavior;
- Circle Pad/C-stick and 3D-slider handling that have no direct DSi equivalent.

These behaviors are reference requirements for the DSi UX, not APIs to copy.

## System/memory clue from the 3DS fork

The 3DS system backend explicitly configures a 25 MiB linear heap and comments that lower values crash at launch. This is **not transferable as a DSi RAM requirement**, because the allocator model, GPU texture allocations, libraries and platform differ. It is nevertheless a warning that the 3DS implementation is not currently designed around a 16 MiB total-RAM machine.

This clue is qualitative only and MUST NOT be used as a GO/NO-GO measurement.

## Primary source URLs

```text
https://github.com/MrHuu/fallout1-ce-3ds/commit/fdb9cd7eca0d03191019c410f084196b6e1df972
https://raw.githubusercontent.com/MrHuu/fallout1-ce-3ds/refs/heads/3DS/Makefile.ctr
https://raw.githubusercontent.com/MrHuu/fallout1-ce-3ds/refs/heads/3DS/src/platform/ctr/ctr_gfx.cc
https://raw.githubusercontent.com/MrHuu/fallout1-ce-3ds/refs/heads/3DS/src/platform/ctr/ctr_input.cc
https://raw.githubusercontent.com/MrHuu/fallout1-ce-3ds/refs/heads/3DS/src/platform/ctr/ctr_rectmap.cc
https://raw.githubusercontent.com/MrHuu/fallout1-ce-3ds/refs/heads/3DS/src/platform/ctr/ctr_sys.cc
```
