# PROJECT_v3.md — Fallout 1 CE Nintendo DSi Port (Feasibility-Gated)

> **V3 change-marking convention**
>
> Only material additions/changes from v2 are tagged with `V3 CHANGE`. Untagged text is inherited from v2.
>
> V3 incorporates eight review changes:
>
> ```text
> #1 melonDS functional preflight, never authoritative timing
> #2 one unified dsi-bench.nds + bench.log
> #3 explicit DS 3D/VRAM/dual-screen constraints
> #4 Phase 0A size result labeled LOWER BOUND when stubbed/incomplete
> #5 overlays treated as verified, separately costed major refactor
> #6 agent subsystem gate separated from full beginning-to-ending validation
> #7 finite three-run Phase 0 engineering budget
> #8 direct Unlaunch memory baseline compared with TWiLight launch
> ```
>
## 0. Agent mandate

You are the primary implementation and measurement agent for this repository.

Your job is NOT to assume that Fallout 1 CE can run fully on a Nintendo DSi.

Your first job is to **prove or disprove feasibility with measurements**.

Do not stop at a prose feasibility report when the required measurements can be obtained by compiling, linking, benchmarking, instrumenting, or running test builds.

Do not claim that the port is possible merely because the code compiles.

Do not claim that it is impossible merely because the desktop/3DS implementation is large.

Use measured numbers wherever possible.

The project has two major phases:

```text
PHASE 0 — FEASIBILITY GATE
    |
    +-- GO  -> continue to full DSi port
    |
    +-- CONDITIONAL GO -> perform explicitly justified architectural changes,
    |                     re-measure, then decide again
    |
    +-- NO-GO -> stop full-port implementation and document the exact physical/
                 architectural blocker with measurements
```

The final desired product, if feasibility is demonstrated, is a fully playable Nintendo DSi port of Fallout 1 Community Edition.

The target is **Nintendo DSi mode**, not ordinary DS mode.

The expected launch environment is normally TWiLight Menu++ or another DSi-mode-capable homebrew launcher.

Do not bundle commercial Fallout assets.

---

# 1. Primary references

Inspect current versions and record exact commit hashes before implementation.

## Fallout 1 CE upstream

```text
https://github.com/alexbatalov/fallout1-ce
```

## Nintendo 3DS port

```text
https://github.com/MrHuu/fallout1-ce-3ds
```

Use the `3DS` branch or the current branch that contains the maintained Nintendo 3DS platform implementation.

The 3DS port is the primary UX/layout reference.

Important areas include:

```text
src/platform/ctr/
Makefile.ctr
```

Audit the actual current tree rather than assuming filenames from this document are exhaustive.

## Nintendo DS/DSi toolchains and documentation

Prefer current supported tooling.

Inspect at least:

```text
https://github.com/devkitPro/libnds
https://github.com/devkitPro/nds-examples
https://github.com/devkitPro/calico
```

Also investigate current BlocksDS documentation/implementations where useful, especially for:

- DSi memory layout
- heap boundaries
- overlays/dynamic loading
- DSi SD access
- ARM7 responsibilities
- graphics hardware
- profiling/debugging

Do not switch toolchains casually. If devkitPro/libnds and BlocksDS offer materially different capabilities relevant to feasibility, compare them and document the decision.

---

# 2. Core engineering principle

This project must be driven by numbers.

The following are NOT acceptable on their own:

```text
"16 MB should probably be enough."
"We can optimize it later."
"The 3DS port works, so DSi should work."
"The DSi is too weak."
"Streaming from SD will solve memory."
"The GPU will solve rendering."
```

Every such claim must be replaced with measured or carefully bounded evidence.

The four mandatory feasibility questions are:

```text
A. How much resident RAM does the engine/code/data require?

B. What is the minimum real runtime working set for an actual playable map?

C. Can the DSi render the required handheld presentation at an acceptable frame time?

D. Can storage/audio/platform abstractions operate within the remaining CPU/RAM/I/O budget?
```

Do not proceed to a full port until these are answered.

---

# 3. Definition of "fully playable"

If Phase 0 produces a GO result, the eventual release may only be described as **fully playable** when all of the following are true on real Nintendo DSi hardware:

- A new game can be started.
- The intro flow can be completed or intentionally skipped without blocking progression.
- The world can be entered.
- Core map exploration works.
- Combat works.
- Dialogue works.
- Inventory works.
- Character screen works.
- Skilldex works.
- Pip-Boy works.
- World map works.
- Barter works.
- Loot works.
- Save works.
- Restart + load works.
- Multiple save slots work.
- Map transitions do not routinely crash or run out of memory.
- Long play sessions do not show progressive heap exhaustion.
- Critical scripted events function.
- **[V3 CHANGE — #6]** Representative progression/save/script regression scenarios pass on real hardware; the agent is NOT required to perform a 30–40 hour beginning-to-ending playthrough as a gate.
- Text is readable on the DSi display strategy.
- Touch/button controls are usable.
- Performance is acceptable enough for normal gameplay.
- User-supplied Turkish-patched data can be tested separately.

The phrase "fully playable" must not be used for:

- a booting shell,
- a title screen,
- a main menu,
- a map viewer,
- a tech demo,
- or a build that only works with audio/game systems removed.

> **[V3 CHANGE — #6: agent gate vs. full-game validation]**
>
> The engineering agent must validate subsystems and representative progression states, but it must **not self-certify a full beginning-to-ending playthrough unless such a playthrough was actually performed and evidenced**.
>
> Full-game validation is a separate release-validation task. It may be performed by the user or other human testers on real DSi hardware. The final public label **fully playable** requires either:
>
> - an evidenced beginning-to-ending human playthrough on real hardware, or
> - an equivalently strong regression suite plus human validation covering the complete progression path.
>
> Phase 0 and the initial port gate do not require the agent to spend dozens of hours playing the game.

---

# 4. PHASE 0 — Mandatory feasibility gate

Phase 0 is the highest priority.

Do not skip it.

Produce:

```text
docs/FEASIBILITY.md
```

and keep it updated with measured values.

The final section must state exactly one of:

```text
GO
CONDITIONAL GO
NO-GO
```

with justification.

> **[V3 CHANGE — #7: Phase 0 effort budget]**
>
> Phase 0 is time-boxed to **three substantive agent runs after the repository and required build toolchain are available**.
>
> A substantive run means a run in which the agent can actually inspect/modify/build/test the repository. Waiting for the user to return a physical-hardware result does not consume a run.
>
> Intended cadence:
>
> ```text
> Run 1:
> repository/toolchain audit + SDL audit + feasibility target +
> lower-bound size result + unified bench artifact/emulator preflight
>
> Run 2:
> real-hardware benchmark ingestion + representative RAM-budget work +
> first GO / CONDITIONAL GO / NO-GO determination
>
> Run 3:
> at most one clearly defined CONDITIONAL-GO architecture experiment,
> re-measurement, and final GO or NO-GO for the current approach
> ```
>
> If, after three substantive runs, the project still cannot produce a defensible GO or NO-GO because of unresolved **technical uncertainty**, this must be reported as a **NO-GO signal for the current approach**, not left open indefinitely.
>
> Do not misclassify missing user hardware results, missing legally supplied Fallout data, or an unavailable external toolchain as proof that the hardware itself is impossible. Label those cases `BLOCKED`, state the missing evidence, and do not consume the Phase 0 run budget while merely waiting.

---

# 5. Phase 0A — Resident binary measurement

## Goal

Determine how much main RAM is consumed before Fallout runtime heap allocations begin.

## Required work

Cross-compile as much of Fallout 1 CE core as possible for the ARM9/DSi target.

Target characteristics should match the actual DSi CPU/toolchain.

Prefer size-oriented compilation for the feasibility build:

```text
-Os
```

Do not use desktop or 3DS-specific compiler flags.

Do not fake the measurement by compiling only trivial files.

If SDL/platform code prevents linkage, provide temporary minimal stubs sufficient to link the real engine code.

The purpose is to obtain an honest size estimate of the engine code and static data.

> **[V3 CHANGE — #4: mandatory LOWER BOUND labeling]**
>
> A stubbed Phase 0A link is **not the final resident binary size**. It is a **LOWER BOUND**.
>
> `docs/SIZE_REPORT.md` must prominently state:
>
> ```text
> RESULT CLASS: LOWER BOUND
> ```
>
> whenever any production subsystem is stubbed, omitted, dead-stripped only because a stub no longer references real code, or not yet linked.
>
> The report must list every material item not included in the number, including as applicable:
>
> ```text
> DSi graphics backend
> rectmap renderer
> input backend
> audio backend
> filesystem/SD glue
> libnds/calico/runtime code
> ARM7-side support code attributable to the application
> diagnostics/logging
> real SDL compatibility shim
> real callbacks that may keep additional --gc-sections code alive
> ```
>
> For each omitted subsystem, give a clearly labeled rough size allowance or range when a defensible estimate can be made. Do not add guesses into the measured total and call the result measured.
>
> Because `--gc-sections` plus minimal stubs can discard code that the real platform implementation will retain, produce at least:
>
> 1. a baseline lower-bound configuration, and
> 2. an optimized lower-bound configuration,
>
> and explain why neither should be treated as the final resident figure until the real DSi platform layer is linked.

Run and record:

```text
arm-none-eabi-size <linked ARM9 binary or ELF>
```

Capture at minimum:

```text
.text
.rodata if separately reported
.data
.bss
total static/resident contribution
```

Also generate a linker map and identify the largest contributors.

Create:

```text
docs/SIZE_REPORT.md
```

It must include:

- exact compiler flags,
- exact toolchain version,
- exact upstream commit,
- total linked binary size,
- static RAM use,
- largest .text contributors,
- largest .rodata contributors,
- largest .data/.bss contributors,
- whether RTTI/exceptions are enabled,
- whether LTO was used,
- whether dead-section elimination was used,
- **[V3 CHANGE — #4]** whether the result is `LOWER BOUND` or `FINAL-LIKE`,
- **[V3 CHANGE — #4]** omitted/stubbed subsystems and estimated size allowance for each,
- **[V3 CHANGE — #4]** a warning if `--gc-sections` may have discarded code only because production callbacks are not yet linked.

Recommended release-size options to evaluate after baseline:

```text
-Os
-ffunction-sections
-fdata-sections
-Wl,--gc-sections
-fno-rtti
-fno-exceptions
-flto
```

Only use flags that are safe for the codebase.

Measure each important configuration instead of assuming benefit.

---

# 6. Phase 0B — Real DSi memory budget

## Goal

Determine the actual usable memory on a real DSi-mode homebrew process.

Do not rely on a guessed "12 MB heap" number.

## Required work

Create a DSi-mode memory-probe `.nds`.

Measure or report using supported APIs/linker symbols:

```text
heap start
heap limit
stack region
static binary end
available heap immediately after startup
largest allocatable block
```

Test in the intended launch environment.

At minimum:

```text
TWiLight Menu++ in DSi mode
```

> **[V3 CHANGE — #8: direct Unlaunch baseline + launcher comparison]**
>
> When Unlaunch is installed and direct launch is available, run the same memory probe **directly from Unlaunch** and compare it with the TWiLight Menu++ launch path.
>
> Do not assume that either launcher remains resident or consumes a fixed amount of RAM. Measure it.
>
> `docs/MEMORY_PROBE.md` must record separately:
>
> ```text
> Direct Unlaunch:
>   heap start
>   heap limit
>   free heap after startup
>   largest allocation
>
> TWiLight Menu++ DSi-mode launch:
>   heap start
>   heap limit
>   free heap after startup
>   largest allocation
>
> Difference:
>   ...
> ```
>
> Unlaunch is the preferred direct-launch baseline for this comparison because it provides a clean, unrestricted DSi homebrew launch path when installed. Do **not** claim that lack of Unlaunch alone proves DSi-mode homebrew impossible; record the exact launcher/exploit path actually used.

Produce the unified benchmark artifact described in §27; the memory probe is one module inside it.

Create/update:

```text
docs/MEMORY_PROBE.md
```

The probe should show the values on-screen and preferably log them to SD.

Do not assume that launcher memory use remains resident unless measured.

---

# 7. Phase 0C — Minimum Fallout runtime working set

## Goal

Measure the minimum memory needed to run a representative real Fallout gameplay state.

This is more important than the static binary size alone.

## Required categories

Instrument and measure, where feasible:

```text
resident engine/static memory
heap baseline after engine init
logical framebuffer(s)
palette data
rectmap/render staging buffers
art cache
map data
critter/object data
script data
message/text data
decompression buffers
file I/O buffers
audio buffers
music/speech decode buffers
save/load temporary buffers
stacks
allocator overhead
platform backend allocations
```

Measure both:

```text
AUDIO OFF
AUDIO ON
```

Do not define the minimum runtime state as an empty menu.

Use at least one real representative gameplay map.

If possible, also test a known heavy map or high-object-count state.

For every category, state:

```text
measured
estimated
unknown
```

Never mix estimates into the measured total without labeling them.

Create:

```text
docs/RAM_BUDGET.md
```

with a table like:

```text
Component                     Bytes       Status
-------------------------------------------------
Resident ARM9 code/data       ...         measured
Engine baseline heap          ...         measured
Framebuffer                   ...         measured
Art cache                     ...         measured
Map working set               ...         measured
Critters/objects              ...         measured
Scripts                       ...         measured
Audio                         ...         measured
I/O/decompression             ...         measured
Safety margin                 ...         required
-------------------------------------------------
Peak total                    ...
```

---

# 8. Safety margin requirement

A build that fits only by a few kilobytes is not feasible.

The feasibility decision must account for allocation spikes and fragmentation.

Prefer a meaningful safety margin.

For Phase 0, target at least:

```text
1 MiB free headroom
```

after loading a representative gameplay state.

If the project can demonstrate through allocator instrumentation that less headroom is safe, document the evidence.

If peak use is effectively at the physical limit, classify the result as:

```text
CONDITIONAL GO
```

or:

```text
NO-GO
```

rather than GO.

---

# 9. Architectural options if RAM is too high

Do not respond with "optimize it."

If RAM exceeds the budget, produce a ranked list of concrete reductions.

For each proposal state:

```text
current measured cost
target cost
expected savings
implementation complexity
performance tradeoff
risk
```

Potential categories include:

## 9.1 Cache reduction

Measure actual art/sound cache requirements.

Reduce cache sizes and re-test.

Do not remove caching entirely without benchmarking resulting SD stalls.

## 9.2 Buffer reuse

Identify buffers that never need to coexist.

Use lifetime analysis to reuse memory.

## 9.3 Indexed rendering

Keep Fallout graphics in 8-bit indexed/paletted form as long as possible.

Avoid unnecessary full-frame RGB expansion.

## 9.4 Audio configuration

Evaluate:

```text
smaller decode buffers
lower sample rate if necessary
streaming strategy changes
speech/music mutual exclusion if justified
```

Do not remove audio permanently merely to make memory numbers pass.

## 9.5 Overlays / dynamic code loading

> **[V3 CHANGE — #5: overlays are a separate architectural investigation, not a cheap fallback]**
>
> Treat overlays/dynamic loading as a **major refactor candidate**, not as a link-time trick.
>
> Fallout CE was not designed as a set of unloadable UI plugins. Before proposing overlays, inspect:
>
> - cross-module function calls,
> - global/static ownership,
> - callback tables,
> - C++ object/vtable lifetimes where present,
> - pointers stored across UI/game transitions,
> - allocator ownership,
> - initialization/destruction ordering.
>
> A safe module boundary may require real engine refactoring.

First verify that the **selected current toolchain** actually supports the required mechanism. Do not assume support because another DS SDK documents it.

Primary reference to verify current BlocksDS dynamic-library behavior:

```text
https://blocksds.skylyrac.net/docs/internal/dynamic_libraries/
```

If the project remains on devkitPro/libnds, verify what overlay/dynamic-loading mechanisms are currently supported there and cite the exact implementation/documentation used.

This is NOT demand paging.

If overlay/dynamic loading is selected, create:

```text
docs/OVERLAY_FEASIBILITY.md
```

before implementing the refactor.

It must include:

```text
toolchain/mechanism verified
source/documentation reference
candidate module boundaries
cross-boundary pointer/callback risks
measured resident bytes expected to be saved
load/unload latency target
SD I/O cost
relocation/code-loading cost
required refactor scope
estimated agent runs
estimated hardware-test iterations
risk rating
```

For each candidate module, prove:

- it can be unloaded safely,
- no live pointers or callbacks into unloaded code remain,
- relocation/loading cost is acceptable,
- code-size savings are material.

Potentially separable areas may be investigated, not assumed separable:

```text
character UI
inventory UI
dialog UI
save/load UI
world map UI
credits
intro/movie support
```

Do not modularize gameplay-hot code unless measurements justify it.

**Overlay work does not automatically qualify as a normal `CONDITIONAL GO` fix.** If the required change is a substantial engine refactor, classify it as a separately priced architectural phase and report its expected effort before spending the Phase 0 budget on it. Only after a prototype demonstrates measured savings may it become the basis for a new feasibility decision.

## 9.6 Feature/code removal

Removing nonessential platform features is acceptable.

Removing required Fallout gameplay systems is not.

Document every removed feature.

---

# 10. Phase 0D — Rendering benchmark

## Goal

Measure actual DSi rendering cost.

Do NOT assume the entire output must be scaled in CPU software.

Nintendo DS/DSi has 2D hardware and a hardware 3D engine.

Evaluate realistic strategies.

## Required strategies

Benchmark at least:

```text
1. CPU software copy/scale baseline
2. 2D-engine-assisted approach where technically suitable
3. 3D-engine textured-quad rectmap approach
```

If one strategy is technically impossible due to texture formats/sizes/VRAM constraints, document exactly why.

> **[V3 CHANGE — #3: mandatory DS/DSi 3D-engine constraint audit]**
>
> Check these constraints in the first rendering investigation instead of discovering them late:
>
> 1. **Texture VRAM capacity.** Verify exact capacities from the selected SDK/hardware reference. The expected DS layout provides VRAM banks A–D as 128 KiB banks that can be mapped for 3D textures, for roughly 512 KiB aggregate texture capacity. A raw `640 x 480 x 8bpp` source is `307,200` bytes before padding/layout constraints.
> 2. **Texture dimensions/layout.** Do not assume a literal 640x480 texture is legal or efficient. Check supported texture dimensions and whether chunking/padding is required.
> 3. **CPU access while mapped as texture VRAM.** Texture-mapped VRAM is not generally writable as ordinary CPU memory. Measure the complete update path, including switching/remapping a bank to CPU/LCDC-visible mode where required, DMA/copying updated indexed data, and remapping it back for 3D use.
> 4. **One-screen 3D limitation.** The DS 3D engine feeds the main graphics engine and cannot independently render both LCDs at once. Dual-screen 3D techniques can require capture/screen alternation and can effectively sacrifice frame rate. Benchmark this; never treat one-screen 3D performance as proof of two-screen performance.
> 5. **Hybrid candidate.** Explicitly benchmark or prototype a configuration where one screen uses the 3D textured-quad rectmap renderer and the other uses a 2D bitmap/background path. This may be preferable to dual-screen 3D.
> 6. **Indexed textures are a preferred candidate.** DS 3D supports paletted/indexed texture formats with texture palettes stored separately. Evaluate this before any full-frame RGB expansion.
>
> Relevant current documentation to verify against:
>
> ```text
> https://blocksds.skylyrac.net/tutorial/intermediate/3d_graphics/
> https://blocksds.skylyrac.net/tutorial/basic/introduction_2d/
> https://blocksds.skylyrac.net/docs/internal/memory_map/
> ```
>
> The benchmark must report the cost of **texture update/remap/upload**, not only polygon submission.

## Test source

Use a representative Fallout-like 8-bit indexed framebuffer.

Preferred source logical size:

```text
640 x 480 x 8bpp
```

However, the DSi renderer does NOT need to upload all 640x480 every frame if dirty rectangles, tiles, or rectmap source regions permit something better.

Evaluate:

```text
full-frame update
dirty-region update
tile/chunk update
rectmap-only update
```

where practical.

## Measure

On real hardware, record:

```text
CPU time per frame
upload/copy time
render submission time
total frame time
FPS
VRAM used
main RAM used
```

Use hardware timers, profiling counters, or best available timing APIs.

Write rendering results to:

```text
docs/RENDER_BENCH.md
```

The executable is the unified `build/dsi-bench.nds` from §27.

**[V3 CHANGE — #1/#2/#3]** Any timing/FPS number produced by melonDS must be labeled `EMULATOR TIMING — INVALID FOR GO/NO-GO`. Only real DSi hardware timing can populate the authoritative performance fields in `RENDER_BENCH.md`.

---

# 11. Target performance thresholds

These are initial engineering gates, not arbitrary marketing claims.

For gameplay:

```text
>= 20 FPS preferred
>= 15 FPS acceptable
10-15 FPS conditional, must be tested for usability
< 10 FPS sustained during ordinary exploration/combat = NO-GO unless a specific measured optimization path exists
```

Menus/dialogue may tolerate lower update frequency if pointer/UI responsiveness remains good.

Measure input latency separately from rendered FPS where useful.

Do not use average FPS alone.

Record worst-case and representative frame times.

---

# 12. DSi rectmap rendering strategy

If feasibility passes, follow the 3DS port's UX architecture.

Do not naively shrink the entire Fallout UI to one DSi screen.

The DSi screens are:

```text
Top:    256 x 192
Bottom: 256 x 192 touchscreen
```

Preserve the original Fallout logical coordinate system where practical.

Port the 3DS rectmap concept:

```text
Fallout framebuffer/source regions
        ->
context-specific mappings
        ->
top and bottom DSi screens
```

Use context-specific layouts for:

```text
field
GUI/HUD
dialogue
inventory
loot
barter
character
Skilldex
world map
save/load
other current 3DS display states
```

Audit the real current 3DS enum and rectmaps.

Do not assume the list above is exhaustive.

---

# 13. Readability gate

"Rendered" is not the same as "usable."

Create a hardware test for UI readability.

Test:

```text
dialogue responses
inventory item names/counts
character stats
Skilldex
Pip-Boy text
save/load names
world map labels
HUD numbers
```

If original-scale text is too small when mapped to 256x192:

- use cropping,
- scrolling,
- context-specific rectmaps,
- zoom,
- or split-screen layout.

Do not solve readability by globally stretching/distorting the image.

Record results in:

```text
docs/UI_READABILITY.md
```

The port cannot be called fully playable if critical text cannot be read comfortably.

---

# 14. Touch/input model

If Phase 0 passes, follow the 3DS port behavior as closely as DSi hardware permits.

Create a DSi input backend.

Required conceptual mapping:

```text
DSi touch coordinate
        ->
active destination rect
        ->
relative position
        ->
source Fallout rect
        ->
Fallout pointer coordinate
```

This must remain correct when UI elements are repositioned.

Initial buttons:

```text
R      = left mouse button
L      = right mouse button
A      = field/GUI screen swap where applicable
SELECT = display/fullscreen mode following 3DS behavior
SELECT + Up   = zoom in
SELECT + Down = zoom out
START  = Escape/back behavior where appropriate
D-pad  = relative cursor or viewport movement
```

Do not invent unnecessary input schemes.

---

# 15. Phase 0E — SDL/API dependency audit

## Goal

Determine the real porting workload.

Do not characterize this as "just add a backend" unless the audit proves it.

Inspect every SDL function/type used by Fallout CE and the 3DS fork.

Create:

```text
docs/SDL_AUDIT.md
```

Classify every used API into:

```text
Rendering
Input
Audio
Timing
Threads
Mutexes/condition variables
Filesystem/path
Events
Surface/palette
Cursor/mouse
Window/display
Miscellaneous
```

For each API, assign:

```text
USE EXISTING DSi IMPLEMENTATION
WRAP WITH LIBNDS/CALICO
CUSTOM SHIM
REMOVE/NO-OP SAFELY
REQUIRES ARCHITECTURAL CHANGE
```

Do not assume a maintained SDL2 implementation exists.

Search current supported DS/DSi ecosystems.

If an SDL2 port exists, evaluate:

- maintenance state,
- DSi mode support,
- RAM cost,
- video backend quality,
- audio backend quality,
- thread support,
- filesystem support.

If it is unsuitable, implement only the SDL subset Fallout CE actually needs.

Prefer a minimal compatibility layer over porting all SDL2.

---

# 16. Threading audit

Determine whether Fallout CE actually requires threads for core gameplay.

For each thread/mutex/condition use:

- identify owner subsystem,
- determine whether it can become single-threaded,
- determine ARM7 suitability if relevant,
- estimate code/RAM complexity.

Do not move arbitrary engine logic to ARM7.

ARM7 has platform responsibilities and limited resources.

---

# 17. Phase 0F — SD I/O benchmark

## Goal

Measure real storage throughput and latency on target hardware.

Do not assume a fixed 1-3 MB/s value.

Write SD results to:

```text
docs/SD_BENCH.md
```

The executable is the unified `build/dsi-bench.nds` from §27.

**[V3 CHANGE — #1/#2]** melonDS may validate SD mount/path logic and log generation, but emulator throughput/latency must never be used as the authoritative `SD_BENCH.md` hardware result or as a GO/NO-GO input.

Measure at minimum:

```text
sequential read:
4 KiB blocks
32 KiB blocks
128 KiB blocks
512 KiB blocks

random-ish reads:
small asset-like reads
mixed seek/read workload

file open/close latency
```

Use the intended SD card and launch environment.

Where possible test:

```text
small SD/SDHC card
the user's actual target SD card
```

Record:

```text
MB/s
latency
CPU involvement
variance
```

---

# 18. Streaming feasibility

If RAM requires asset streaming, build a representative benchmark.

Simulate or use real Fallout asset reads during:

```text
map transition
inventory opening
dialog opening
art animation
```

Measure visible stalls.

Initial usability targets:

```text
ordinary UI action stall < 250 ms preferred
map transition < 3 s preferred
3-6 s conditional
> 6 s routine map transition requires strong justification
```

These are practical targets, not absolute physical limits.

---

# 19. Audio feasibility

Do not make audio the first milestone.

First prove:

```text
engine
graphics
input
filesystem
memory
```

Then benchmark audio.

Measure:

```text
decoder memory
streaming buffer memory
CPU cost
SD bandwidth
underruns
```

Test:

```text
sound effects
speech
music
combined load
```

If necessary, evaluate configurable reductions:

```text
buffer size
sample rate
channel count
decode strategy
```

Do not permanently remove audio to declare success.

---

# 20. Phase 0 decision table

After measurements, classify the project.

## GO

Use GO only if:

```text
- resident code/static memory fits comfortably;
- representative gameplay peak memory fits with safety margin;
- rendering meets usable frame-time targets;
- SD behavior is acceptable;
- SDL/platform requirements have a concrete implementation path;
- no known subsystem requires more RAM/CPU than remains.
```

Then continue immediately to full-port implementation.

## CONDITIONAL GO

Use when feasibility is plausible but depends on a specific architectural change.

Examples:

```text
- split 1-2 UI subsystems into overlays to save measured resident code;
- reduce art cache from X to Y and measured stalls remain acceptable;
- use hardware textured-quads instead of software scaling;
- replace SDL audio with a small native audio backend;
```

For CONDITIONAL GO:

1. implement the proposed change,
2. re-run the relevant benchmark,
3. update FEASIBILITY.md,
4. decide GO or NO-GO.

Do not leave CONDITIONAL GO indefinitely.

## NO-GO

Use NO-GO when a hard constraint remains after reasonable architecture changes.

Examples:

```text
- minimum representative runtime working set exceeds physical RAM;
- required performance remains <10 FPS with no credible measured optimization path;
- storage stalls make normal gameplay unusable;
- required platform subsystem cannot be implemented within remaining resources.
```

NO-GO documentation must say exactly which physical limit was exceeded and by how much.

---

# 21. Full-port phase after GO

Only after GO should the project switch into full implementation mode.

The full port must still use measurements continuously.

Desired platform layout:

```text
src/platform/dsi/
    dsi_gfx.*
    dsi_input.*
    dsi_rectmap.*
    dsi_sys.*
    dsi_audio.*
    dsi_fs.*
```

Exact filenames may differ.

Keep DSi-specific code isolated from gameplay code wherever practical.

---

# 22. Build target

Create a reproducible DSi build.

Preferred:

```bash
make -f Makefile.dsi
```

or a clearly documented equivalent.

The normal output should eventually be:

```text
build/fallout1-dsi.nds
```

The build must target DSi mode.

If launched in DS mode, fail gracefully:

```text
Fallout 1 CE DSi requires DSi mode.
```

Do not crash or silently continue in an unsupported mode.

---

# 23. Game data layout

Preferred runtime structure:

```text
sd:/
  fallout1/
    fallout1-dsi.nds
    fallout.cfg
    master.dat
    critter.dat
    data/
    saves/
```

If another path is technically superior, document it.

Never bundle:

```text
master.dat
critter.dat
commercial art
commercial audio
translated commercial game assets
```

Missing assets should produce clear on-screen diagnostics.

---

# 24. Turkish support

Turkish support is a post-core-port requirement.

The engine must not become Turkish-only.

Test using a user-provided legally obtained Turkish-patched Fallout installation.

Audit whether the translation modifies:

```text
master.dat
data/text
font files
art
fallout executable
external loader/patcher
```

Required Turkish characters:

```text
ç Ç
ğ Ğ
ı İ
ö Ö
ş Ş
ü Ü
```

Create:

```text
docs/TURKISH.md
```

If the translation requires executable patching, determine what behavior must be reimplemented in the CE engine.

Do not redistribute copyrighted translation/game files without permission.

---

# 25. Logging and crash diagnostics

Real hardware iteration is mandatory.

Implement useful diagnostics early.

Preferred log:

```text
sd:/fallout1/fallout-dsi.log
```

Record important events only.

Include:

```text
startup stage
heap free
peak heap
asset load failures
map being loaded
last successful subsystem
fatal error
```

On fatal error, show useful information on-screen.

---

# 26. Hardware test protocol

> **[V3 CHANGE — #1: melonDS in the iteration loop]**
>
> Before asking for a physical DSi round-trip, run every benchmark/test that can provide **functional correctness evidence** in melonDS configured for DSi emulation when the required user-provided DSi BIOS/firmware/NAND and virtual SD environment are available.
>
> melonDS is appropriate for preflighting:
>
> ```text
> dsi-bench.nds boot flow
> DSi-mode path assumptions
> SD mount/path logic
> bench.log creation
> crash-stage/error codes
> touchscreen/button flow
> rectmap coordinate correctness
> asset detection/path handling
> startup/error screens
> deterministic non-timing unit/integration behavior
> ```
>
> **Hard rule: melonDS is NOT an authoritative timing benchmark.**
>
> Do not use melonDS values for:
>
> ```text
> FPS
> ms/frame
> CPU cycle budget
> VRAM upload timing
> SD MB/s
> SD latency
> audio underrun timing
> map-transition wall-clock performance
> ```
>
> Such fields must be marked:
>
> ```text
> EMULATOR TIMING — INVALID FOR GO/NO-GO
> ```
>
> `docs/RENDER_BENCH.md` and `docs/SD_BENCH.md` may contain emulator numbers only in a clearly separated diagnostic appendix. Their authoritative performance tables must come from real Nintendo DSi hardware.
>
> melonDS official DSi emulation requires user-provided DSi BIOS, firmware, and NAND. Do not download or redistribute proprietary dumps.
>
> Reference:
>
> ```text
> https://melonds.kuribo64.net/faq.php
> ```

Whenever the next unknown requires a physical DSi, produce:

```text
docs/HARDWARE_TEST.md
```

It must state exactly:

```text
which .nds to copy
where to copy it
which data files are required
what to press/touch
expected output
what numbers/errors to report
```

Do not ask:

```text
"Does it work?"
```

Ask for concrete observations.

Example:

```text
Report:
- heap free after boot:
- sequential 128 KiB read MB/s:
- render benchmark ms/frame:
- crash stage code:
```

---

# 27. Initial benchmark artifacts

> **[V3 CHANGE — #2: one benchmark artifact, one hardware round-trip]**
>
> Phase 0 must use a single integrated benchmark executable:

```text
build/dsi-bench.nds
```

On boot it should run, in a deterministic sequence or simple on-device menu, these modules:

```text
1. environment / DSi-mode / launcher diagnostics
2. memory probe
3. SD mount + path + log test
4. SD benchmark
5. render/VRAM benchmark
6. input/touch sanity test
7. rectmap coordinate self-test
8. crash-stage/fatal-screen test path where safe
```

The artifact must print a concise result summary on-screen and write:

```text
sd:/fallout1/bench.log
```

Prefer a log format that is easy to paste back into an agent, for example:

```text
BENCH_VERSION=...
BUILD_COMMIT=...
RUN_ENV=REAL_DSI|MELONDS
LAUNCHER=UNLAUNCH|TWILIGHT|OTHER
DSI_MODE=1

[MEMORY]
...

[SD]
...

[RENDER]
...

[INPUT]
...

[RECTMAP]
...

[ERRORS]
...
```

The user should need to:

```text
copy one .nds
launch once
return one bench.log
```

If a test can crash before the log is flushed, checkpoint the last completed stage safely without writing continuously every frame.

`docs/MEMORY_PROBE.md`, `docs/SD_BENCH.md`, and `docs/RENDER_BENCH.md` remain separate analysis documents, but all three consume results from this one executable.

Run the same artifact in melonDS first for functional preflight, then on real hardware for authoritative timing/I/O/memory-environment measurements.

If the environment cannot build `.nds` files because the toolchain cannot be installed, still implement the unified source/build target and document the exact dependency issue.

Never fabricate build success.

---

# 28. Git workflow

Use Git.

Make focused commits.

Examples:

```text
dsi: add feasibility build target
dsi: add memory probe
dsi: add hardware render benchmark
dsi: add SD benchmark
audit: document SDL API usage
dsi: add indexed texture prototype
dsi: add rectmap coordinate tests
```

Do not mix unrelated cleanup into platform commits.

Preserve upstream history if practical.

---

# 29. Required documentation

Maintain:

```text
README_DSI.md

docs/FEASIBILITY.md
docs/SIZE_REPORT.md
docs/MEMORY_PROBE.md
docs/RAM_BUDGET.md
docs/RENDER_BENCH.md
docs/SD_BENCH.md
docs/SDL_AUDIT.md
docs/UI_READABILITY.md
docs/HARDWARE_TEST.md
docs/DSI_DEVIATIONS.md
docs/TURKISH.md
docs/AUDIO.md
docs/OVERLAY_FEASIBILITY.md   # only if §9.5 is actually investigated
```

Do not create empty placeholder documents merely to satisfy the list.

Create them as their stage becomes relevant.

---

# 30. 3DS-port parity policy

If feasibility passes, use the 3DS port as the default behavior reference for handheld UX.

Preserve where possible:

```text
rectmap architecture
field vs GUI display modes
dialog layout concept
inventory/loot/barter specialized layouts
character screen layout
Skilldex layout
save/load layout
touch -> Fallout coordinate conversion
screen swap
zoom
mouse-button mapping
```

Any material DSi deviation must be recorded in:

```text
docs/DSI_DEVIATIONS.md
```

with:

```text
3DS behavior
DSi behavior
reason
possible future improvement
```

---

# 31. Important hardware reality checks

The agent must explicitly account for:

```text
16 MiB DSi main RAM
ARM9 resident executable/static data
no desktop-style demand paging
limited CPU compared with 3DS
no FPU assumption
limited VRAM
ARM7 platform responsibilities
SD I/O latency
small 256x192 displays
```

However:

Do not incorrectly assume that DSi rendering must be entirely software.

Investigate and benchmark:

```text
2D hardware
3D textured polygon engine
palette/indexed texture options
DMA
VRAM layout
dirty rectangles
```

Likewise, do not assume every code section must remain resident if supported overlay/dynamic-loading mechanisms can safely reduce resident code.

These mechanisms must be measured, not hand-waved.

---

# 32. First execution sequence

Execute in this order.

1. Read this file fully.
2. Inspect current repository state.
3. Inspect exact Fallout 1 CE upstream commit.
4. Inspect exact 3DS port commit.
5. Record both hashes.
6. Audit build dependencies.
7. Audit SDL usage.
8. Add a DSi feasibility build target.
9. Cross-compile/link the maximum realistic engine core.
10. Produce `arm-none-eabi-size` and linker-map results.
11. Identify the largest static/code contributors and publish `docs/SIZE_REPORT.md` with the mandatory **LOWER BOUND** status where applicable.
12. **[V3 CHANGE — #2]** Build the unified `build/dsi-bench.nds` containing memory, SD, render, input, rectmap, environment, and logging tests.
13. **[V3 CHANGE — #1]** Preflight `dsi-bench.nds` in melonDS DSi mode when legal user-provided DSi firmware/NAND inputs are available; fix boot/path/log/rectmap/crash-stage issues there first.
14. **[V3 CHANGE — #1]** Ignore melonDS timing/I/O performance as a feasibility result; mark it emulator-only.
15. Add/execute host-side rectmap coordinate tests where useful in addition to the on-device self-test.
16. **[V3 CHANGE — #8]** Obtain one real-hardware `bench.log`, preferably once from direct Unlaunch and once from TWiLight Menu++ DSi-mode launch for the memory comparison.
17. Construct the measured RAM budget.
18. Produce a GO / CONDITIONAL GO / NO-GO decision.
19. If CONDITIONAL GO, implement only a specific, budgeted architecture change and re-measure.
20. If the candidate change is a major overlay/dynamic-loading refactor, use §9.5 and price it as a separate architectural phase rather than silently consuming the normal Phase 0 budget.
21. If GO, continue directly into the real Fallout port.
22. If NO-GO, stop pretending a full port is viable and document the exact blocker.

> **[V3 CHANGE — first-run acceptance requested by reviewer]**
>
> The first serious run is expected to complete **steps 1–11**, plus create the unified `dsi-bench.nds` target and run it in melonDS if that emulator environment is available. The first `SIZE_REPORT.md` must explicitly identify lower-bound status and missing platform costs.

Do not stop after step 7 with a report.

---

# 33. Minimum acceptable output of the first serious agent run

The first serious run should leave concrete engineering artifacts.

Preferred minimum:

```text
- exact upstream/3DS commit hashes documented;
- DSi build target created;
- linked ARM9 size measurement produced or exact blocker shown;
- linker map analyzed;
- SDL usage inventory started/completed;
- **[V3 CHANGE — #2]** unified `dsi-bench.nds` source/build artifact produced, containing memory/render/SD/input/rectmap tests;
- **[V3 CHANGE — #1]** melonDS functional preflight performed when a configured DSi environment is available;
- rectmap mapping test implemented;
- FEASIBILITY.md updated with measured vs unknown values;
- next required physical-hardware measurements clearly specified.
```

A prose-only answer is not a successful run unless the execution environment genuinely prevents all compilation/implementation work.

---

# 34. Completion report format

At the end of each major run, report:

```text
Measured:
Implemented:
Built:
Tested:
Unknown:
Current feasibility status:
Current blocker:
Next highest-value action:
Hardware test required:
```

Use exact numbers when available.

Example:

```text
Measured:
- ARM9 .text: 2.31 MiB
- .data+.bss: 0.74 MiB
- free heap after startup: 12.86 MiB
- render prototype: 7.4 ms/frame
- 128 KiB sequential SD read: 2.8 MiB/s

Current feasibility status:
CONDITIONAL GO

Current blocker:
Representative map + art working set peaks 0.9 MiB above the required safety margin.
```

Do not use vague statements such as:

```text
"memory looks okay"
"performance seems fine"
```

---

# 35. Final instruction

Do not begin from the assumption that the port is possible.

Do not begin from the assumption that it is impossible.

**Measure it.**

If the measured feasibility gate passes, implement the full port.

If it fails, identify exactly why, quantify the gap, and only continue if a specific architectural change has a credible measurable path to closing that gap.

The user's goal is not a portfolio demo.

The user's goal is to actually play Fallout 1 on a Nintendo DSi.
