# Makefile.dsi feasibility target notes

## Purpose

`Makefile.dsi` is intentionally a Phase 0A lower-bound target, not the production DSi port build.

It compiles the same Fallout core source directories listed by the maintained 3DS Makefile:

```text
src
src/game
src/int
src/int/support
src/plib/assoc
src/plib/color
src/plib/db
src/plib/gnw
third_party/fpattern/vendor
```

It deliberately excludes:

```text
src/platform/ctr
```

and adds:

```text
src/platform/dsi/feasibility
```

## Toolchain model

Uses the official devkitPro DS ARM9 model:

```text
-march=armv5te
-mtune=arm946e-s
-specs=ds_arm9.specs
-lnds9
```

Size-oriented flags required by PROJECT_v3:

```text
-Os
-ffunction-sections
-fdata-sections
-Wl,--gc-sections
```

Linker-map output:

```text
-Wl,-Map,<absolute repo path>/build/fallout1-dsi-feasibility.map
```

The resulting ELF contract is exactly:

```text
build/fallout1-dsi-feasibility.elf
```

`third_party/fpattern/vendor` contains the exact `v1.9` source already pinned by
the desktop CMake build (`96f42d01b0879a0f8938ec9cb6046209c4a19f3d`).
Vendoring these three small source files keeps the cross build deterministic;
the Makefile cannot use CMake FetchContent during an offline or isolated build.

## SDL/platform shim

Files:

```text
src/platform/dsi/runtime/include/SDL.h
src/platform/dsi/runtime/dsi_sdl.cc
```

These provide enough SDL2-shaped types/constants/functions to move compilation beyond the obvious desktop SDL dependency and expose deeper ARM9 incompatibilities.

They are intentionally not a real renderer/input/audio layer.

A link produced with them is always:

```text
LOWER BOUND
```

## Expected first-CI behavior

The first CI run may fail. That is useful.

Likely next categories of real errors include:

```text
missing SDL symbols/types not yet covered by the shim
newlib/POSIX compatibility gaps
std::recursive_mutex/thread-runtime behavior
zlib/package availability
DS-specific filesystem assumptions
host-only platform assumptions in core files
ARM/ABI compile issues
```

Fix the first real compiler/linker errors iteratively; do not weaken the target to compile only trivial engine subsets just to obtain a small number.
