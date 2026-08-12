# RENDER_BENCH — Phase 0D design

## Result status

```text
AUTHORITATIVE REAL-DSi TIMING: NOT MEASURED
EMULATOR TIMING: NOT A GO/NO-GO INPUT
```

## Verified constraints

- Retail DSi main RAM is 16 MiB. This does not establish usable heap.
- VRAM banks A-D are 128 KiB each and can provide 512 KiB of 3D texture
  storage in aggregate.
- libnds 2.0.2 exposes power-of-two texture dimensions from 8 through 1024.
  A literal 640x480 texture therefore pads to 1024x512 at 8 bpp, consuming all
  524,288 texture bytes. It is legal in dimensions but a poor update layout.
- `GL_RGB256` is an indexed 8-bpp texture format with a separate palette and is
  the preferred first 3D candidate.
- The CPU cannot update texture-mapped VRAM while the 3D engine uses it. The
  complete timed path must switch affected banks to LCDC, copy/DMA, then map
  them back to texture slots.
- Safe live edits have a short VBlank window: current BlocksDS documentation
  identifies scanlines 192 through 213, only 22 scanlines before 3D begins.
- The 3D engine feeds only the main graphics engine. Dual-screen 3D requires
  capture/screen alternation and sacrifices roughly half the application frame
  opportunities; one-screen results cannot stand in for dual-screen results.
- The sub engine is 2D-only. A hybrid main-3D/sub-2D path is therefore a
  mandatory candidate.

Primary references:

```text
https://github.com/devkitPro/libnds/blob/v2.0.2/include/nds/arm9/videoGL.h
https://blocksds.skylyrac.net/tutorial/intermediate/3d_graphics/
https://blocksds.skylyrac.net/tutorial/basic/introduction_2d/
https://blocksds.skylyrac.net/docs/internal/memory_map/
```

## Source and layouts

The deterministic source is a 640x480 indexed framebuffer plus a 256-entry
palette. It uses a moving Fallout-like dirty pattern so copies cannot be
optimized away.

The preferred full-source 3D layout is six 256x256 `GL_RGB256` chunks in a 3x2
grid. It covers a padded 768x512 area for 393,216 bytes, leaving 131,072 bytes
of A-D texture VRAM. Rectmaps reference only the chunk portions they need.

The benchmark must also test a demand-loaded rectmap set that uploads only the
source chunks visible on the two LCD layouts. Its residency and upload bytes
must be logged separately from the six-chunk case.

## Required benchmark matrix

Every case runs warm-up frames, then at least 600 measured frames, recording
median, p95 and worst frame time rather than average FPS alone.

| ID | Strategy | Update set | Required timing split |
|---|---|---|---|
| CPU-FULL | CPU nearest-neighbor baseline to LCD-sized buffers | full 640x480 source | scale, copy/upload, total |
| 2D-FULL | paletted 2D bitmap/affine background | full visible regions | staging, DMA, register update, total |
| 2D-DIRTY | same 2D path | 32x32, 64x64, 128x64 dirty regions | staging, DMA, total |
| 3D-FULL | six indexed texture chunks + rectmap quads | all source chunks | LCDC remap, upload, remap-back, submission, total |
| 3D-DIRTY | same 3D path | dirty chunks/rectangles | remap, upload bytes, submission, total |
| HYBRID | main screen 3D; sub screen paletted 2D | representative field+GUI layouts | per-screen upload, submission, total |
| DUAL3D | capture/screen-alternating 3D | representative dual layout | capture/alternation and effective FPS |

CPU-FULL is a lower-performance baseline, not the intended renderer. DUAL3D is
included to quantify its penalty, not because it is the preferred design.

## Measurement contract

For each case `bench.log` must contain:

```text
RUN_ENV=REAL_DSI|MELONDS
RENDER_CASE=...
SOURCE_BYTES=307200
TEXTURE_VRAM_BYTES=...
PALETTE_VRAM_BYTES=...
MAIN_RAM_STAGING_BYTES=...
UPLOAD_BYTES_PER_FRAME=...
REMAP_TICKS=...
UPLOAD_TICKS=...
SUBMIT_TICKS=...
TOTAL_TICKS=...
FRAME_US_MEDIAN=...
FRAME_US_P95=...
FRAME_US_WORST=...
FPS_EFFECTIVE=...
MISSED_VBLANKS=...
WHITE_TEXTURE_FAULTS=...
```

Hardware timer ticks and the exact conversion constant/toolchain version must
be logged. Cache maintenance, DMA completion waits, VRAM mapping and `glFlush`
belong inside the timed interval. A test that times polygon submission but not
the texture update path is invalid.

melonDS may validate boot, layout, dirty-rectangle selection, coordinate
mapping and log format. Any emulator timing field must be printed as:

```text
EMULATOR TIMING — INVALID FOR GO/NO-GO
```

## Decision use

The hybrid and best single-screen strategy advance only if representative and
p95 frame time support at least 15 FPS and ordinary worst cases do not sustain
below 10 FPS. VRAM fit alone is not a GO result.
