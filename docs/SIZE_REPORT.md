# SIZE_REPORT — Phase 0A ARM9 lower bound

## Result class

```text
LOWER BOUND — successful ARM9 compile and link
```

The final verified result is from GitHub Actions run `31626859493`, commit
`cab6320979593cdefe9fa41ab5db388b417068e6`, artifact `9153492831`. The
successful artifact contains both ELF files, both linker maps, and populated
`arm-none-eabi-size`/`-A` output. The generated ELF/map evidence has also been
unpacked under the local ignored `build/` directory.

## Reproducible environment

- Container: `devkitpro/devkitarm:20260610`
- devkitARM: `r68-1`
- GCC/binutils packages: `devkitarm-gcc 16.1.0-1`,
  `devkitarm-binutils 2.46.0-1`
- libnds: `2.0.2-1`
- Calico: `1.2.0-1`
- NDS zlib: `1.3-1`
- Architecture: `-march=armv5te -mtune=arm946e-s`
- C++ policy: GNU++17, `-Os`, no RTTI, no exceptions, no LTO
- Runtime link: Calico `ds9.specs`, `libnds9`, `libcalico_ds9`

Baseline omits per-function/data sections and linker garbage collection.
Optimized adds `-ffunction-sections -fdata-sections` and
`-Wl,--gc-sections`.

## Measured section totals

Values are bytes from GNU `size` against the ARM9 ELF artifact.

| Configuration | text | data | bss | total (`dec`) | Difference vs baseline |
|---|---:|---:|---:|---:|---:|
| Baseline | 902,600 | 247,036 | 1,519,792 | 2,669,428 | — |
| Optimized | 895,024 | 246,916 | 1,518,904 | 2,660,844 | -8,584 |

The optimized static footprint is about 2.54 MiB. Relative to nominal 16 MiB
DSi RAM it leaves 14,116,372 bytes (about 13.46 MiB) before heap allocations,
stacks, production platform backends, runtime caches, decoded assets, audio,
filesystem buffers, and benchmark logging. This subtraction is not a working-set
measurement and is not evidence that the 1 MiB runtime-headroom gate passes.

The optimized allocated-section detail includes:

| Output section | Bytes |
|---|---:|
| `.secure` | 2,048 |
| `.bootstub` | 408 |
| `.crt0` | 960 |
| `.vectors` | 32 |
| `.itcm` | 1,604 |
| `.dtcm.bss` | 152 |
| `.main` | 890,800 |
| `.main.rw` | 244,812 |
| `.main.bss` | 1,518,752 |
| ARM exception/unwind and init/fini arrays | 1,216 |

Debug sections are present in the ELF but are not resident sections and are not
included in the `text + data + bss` total.

## Largest optimized core contributors

These figures aggregate allocated input sections by core object in the linker
map. Runtime library contributions are excluded from this ranking.

### Code and read-only data

| Object | Bytes |
|---|---:|
| `src/movie_lib.cc` | 50,472 |
| `src/game/editor.cc` | 44,078 |
| `src/game/inventry.cc` | 37,249 |
| `src/game/object.cc` | 26,525 |
| `src/int/support/intextra.cc` | 24,974 |
| `src/game/worldmap.cc` | 24,554 |
| `src/game/gdialog.cc` | 22,092 |
| `src/game/combat.cc` | 19,811 |

### Initialized writable data

| Object | Bytes |
|---|---:|
| `src/game/worldmap_walkmask.cc` | 199,500 |
| `src/game/combat.cc` | 24,338 |
| `src/game/perk.cc` | 3,840 |
| `src/game/worldmap.cc` | 2,310 |

The 199,500-byte world-map walk mask is embedded as writable data. Moving it to
an on-demand or immutable representation is a concrete later optimization
candidate, not a Phase 0 measurement adjustment.

### Zero-initialized data

| Object | Bytes |
|---|---:|
| `src/game/light.cc` | 480,000 |
| `src/plib/color/color.cc` | 231,577 |
| `src/game/anim.cc` | 177,915 |
| `src/game/object.cc` | 173,647 |
| `src/game/map.cc` | 120,605 |
| `src/int/export.cc` | 93,196 |
| `src/game/gdialog.cc` | 34,070 |
| `src/game/fontmgr.cc` | 33,037 |

The largest single item is the three-elevation light-intensity grid:
`int tile_intensity[ELEVATION_COUNT][HEX_GRID_SIZE]` (480,000 bytes).

## Why this remains a lower bound

The target compiles all 110 selected Fallout/fpattern core translation units
and one SDL feasibility shim, but does not yet link production DSi graphics,
input, audio, filesystem/SD, benchmark/logging, or ARM7 application support.
The shim also changes callback reachability, and optimized section garbage
collection can discard code that a production backend will retain.

No unmeasured subsystem allowance is invented. Each remains `unknown` until a
real implementation or benchmark provides evidence.

## Decision

Phase 0A compile/link feasibility passes. Overall DSi feasibility remains
`UNKNOWN`, not `GO`: Phase 0C must measure representative peak working set and
fragmentation, and Phase 0D must measure real-hardware render/VRAM upload cost.
