# RAM_BUDGET — Phase 0C instrumentation plan

## Status

No representative gameplay run has been measured yet. Every numeric gameplay
field is therefore `unknown`; the 16 MiB hardware capacity is a platform fact,
not a measured free-heap result. The unified benchmark now adds a user-supplied
real-data lower bound: it parses both DAT catalogs and simultaneously loads a
representative map, script list/bytecode, critter/item prototypes, message,
palette, interface art and critter art.
This is explicitly not counted as a gameplay working set.

The first instrumentation hook is implemented in
`src/plib/gnw/memory.{h,cc}`. `mem_get_stats` reports current/peak block counts
and current/peak bytes, including GNW guard and alignment overhead.
`mem_reset_peak_stats` starts a new peak window without hiding live bytes.

## Budget table

| Component | Bytes | Status | Measurement source |
|---|---:|---|---|
| Resident ARM9 lower-bound text/data/bss | 2,660,844 | measured lower bound | optimized Phase 0A ELF + map |
| Engine baseline heap | unknown | unknown | GNW stats after engine init |
| Logical 640x480x8 framebuffer | 307,200 | calculated | fixed allocation requirement |
| Palette | 1,024 in SDL-shaped form; production format TBD | calculated/unknown | 256 RGBA entries now; DSi palette design pending |
| Render staging and rectmap buffers | unknown | unknown | DSi renderer counters |
| Art cache | unknown | unknown | `art_cache` and its `Heap` fields |
| Map data | unknown | unknown | tagged checkpoint delta |
| Critters/objects | unknown | unknown | tagged checkpoint delta |
| Scripts | unknown | unknown | tagged checkpoint delta |
| Message/text data | unknown | unknown | tagged checkpoint delta |
| I/O/decompression buffers | unknown | unknown | DB/LZSS + decoder counters |
| Audio OFF | unknown | unknown | gameplay peak window |
| Audio ON | unknown | unknown | gameplay peak window + direct audio allocations |
| Stacks and allocator overhead outside GNW | unknown | unknown | linker symbols + platform heap probe |
| Platform/backend allocations | unknown | unknown | DSi backend counters |
| Real DAT catalogs and representative asset payload | unknown | implemented, hardware pending | `dsi-bench.nds` ORIGINAL/TURKISH checkpoints |
| Required free headroom | 1,048,576 | required | PROJECT_v3 §8 |
| Peak total | unknown | unknown | maximum simultaneous resident set |

Calculated values are not included in a measured total until the allocation is
observed in the DSi build.

## Instrumentation coverage and gaps

Most engine allocations already converge on `mem_malloc`:

- `gmemory_init` routes assoc, DB and interpreter allocation through it;
- GNW registers `gmalloc/grealloc/gfree`;
- `gsound` registers the sound manager allocator;
- GNW registers the color allocator;
- art and sound caches use the engine `Heap`, whose live/free/locked/system
  fields can be sampled directly.

The following direct libc allocations are not covered by `mem_get_stats` and
must be counted separately or routed through a DSi tracking allocator before a
GO/NO-GO total is accepted:

- `audio_engine.cc` sound-buffer storage;
- `sound_decoder.cc` decoder state, 128 KiB scale table and sample buffers;
- feasibility SDL objects (not part of the production total);
- future libnds/calico, filesystem, renderer and logging allocations.

## Required checkpoint sequence

The DSi runtime logger will emit one line per checkpoint with current bytes,
peak bytes, current blocks, largest allocatable block, platform free heap and
the active category. Required checkpoints are:

1. process entry, before engine initialization;
2. after platform/filesystem initialization;
3. after engine initialization (still unknown; archive-catalog checkpoint is separate);
4. after the 640x480 indexed framebuffer and render staging are allocated;
5. immediately before representative map load;
6. after map, objects, scripts and messages are resident (asset-payload lower bound implemented; parsed runtime state still unknown);
7. after art preloading settles (representative raw art payload implemented; production art cache still unknown);
8. after 60 seconds of representative exploration with audio off;
9. after combat and inventory/dialog transitions with audio off;
10. repeat the same state with audio on;
11. save and load peak windows;
12. after map unload, to expose retained or leaked memory.

Each checkpoint must include both subsystem counters and total platform heap
state. Deltas are attribution evidence; only the simultaneous total and its
peak are used for feasibility.

## Representative-state acceptance

The empty menu is not accepted. The first authoritative run needs a real map
with the player, objects, scripts, messages and art cache active. A known
high-object-count/heavy state must follow if one is legally available.

Report two separate peaks:

```text
AUDIO_OFF_PEAK_BYTES=...
AUDIO_ON_PEAK_BYTES=...
```

Also report the smallest observed largest-free-block value so fragmentation is
not hidden by aggregate free bytes. GO requires at least 1 MiB remaining after
the representative peak unless allocator evidence justifies a smaller margin.
