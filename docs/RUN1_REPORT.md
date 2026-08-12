# Run 1 report — PROJECT_v3 §34

Measured:
- No authoritative ARM9 ELF size or physical-DSi performance measurement yet.
- Current sandbox still lacks `arm-none-eabi-g++`, `arm-none-eabi-size`, and melonDS.
- Previous network audit established general outbound shell DNS blocking rather than a Git-only failure.
- Docker Hub publication was verified and CI is pinned to `devkitpro/devkitarm:20260610`.
- Existing 3DS package-size pre-signal remains `INDICATIVE — NOT MEASURED, DIFFERENT ARCH/SDK` and is excluded from GO/NO-GO.

Implemented:
- Added `Makefile.dsi` with an explicit `feasibility` target.
- ARM9 target uses `-march=armv5te -mtune=arm946e-s`; no 3DS ARMv6K/hard-float flags.
- Added required `-Os`, function/data sections and `--gc-sections`.
- Added exact map output `build/fallout1-dsi-feasibility.map`.
- Added exact ELF output `build/fallout1-dsi-feasibility.elf`.
- Added minimal SDL2-shaped Phase-0 compile/link shim under `src/platform/dsi/feasibility`.
- Closed the checked-out SDL surface needed by all 111 feasibility translation units.
- Vendored fpattern v1.9 at commit `96f42d01b0879a0f8938ec9cb6046209c4a19f3d` because the Makefile cannot use CMake FetchContent.
- Added baseline and optimized lower-bound configurations.
- Updated GitHub Actions workflow to pinned `devkitpro/devkitarm:20260610`.
- Added container Git `safe.directory` configuration.
- Added explicit libnds and `ndstool` checks.
- Added `dkp-pacman -S --needed --noconfirm nds-dev` recovery when DS prerequisites are absent.
- Added `nds-zlib` recovery when the DS zlib portlib is absent.
- Preserved failed-build logs and always-uploaded size/map/ELF/SDL inventory artifacts.
- Updated local/container `scripts/ci-dsi-size.sh` to match the workflow.
- Included `CTR_AUDIT.md` and `SDL_AUDIT.md` in the handoff.
- Reordered `FEASIBILITY.md` next evidence by information value: Phase 0D render first, Phase 0C working set second, Phase 0A CI in parallel.
- Added `RAM_BUDGET.md`, exposed GNW current/peak allocation counters and documented uncovered direct allocations.
- Added `RENDER_BENCH.md` with CPU, 2D, 3D, dirty-update, hybrid and dual-3D cases.

Built:
- No ARM9 ELF built in this sandbox because devkitARM is unavailable locally.
- The feasibility build target now exists and can be executed immediately in CI/local devkitPro.
- A host-only closure ELF linked successfully; its size is not an ARM9 result.

Tested:
- `scripts/ci-dsi-size.sh` passes `bash -n` syntax validation.
- `git diff --check` passes.
- `Makefile.dsi` parses and exposes the `feasibility` target under a synthetic ds_rules dry-run harness.
- All 111 feasibility translation units compile and link with the host compiler and shim; real ARM9 compilation remains authoritative.

Unknown:
- First real ARMv5TE compiler error in the checked-out Fallout source.
- Whether the initial shim covers every SDL type/symbol needed to reach link stage.
- ARM9 `.text/.rodata/.data/.bss`.
- Largest linker-map contributors.
- `std::recursive_mutex`/thread-runtime compatibility on the chosen DS toolchain.
- Real DSi free heap and fragmentation behavior.
- Real map/art/script/audio working set.
- Real DSi VRAM remap/upload timing and sustainable FPS.
- Real SD throughput/latency.

Current feasibility status:
BLOCKED — Run 1 budget NOT consumed.

Current blocker:
This sandbox cannot execute devkitARM or melonDS. GitHub Actions must now
provide the ARM9 compiler/linker evidence.

Next highest-value action:
Push the current branch, inspect the `DSi Phase 0 size feasibility` workflow and
iterate its ARM9 errors. In parallel, turn the recorded Phase 0D matrix into the
unified `dsi-bench.nds` implementation.

Hardware test required:
Not for the Phase 0A CI run. A real DSi becomes mandatory as soon as the unified Phase 0D renderer benchmark is buildable; melonDS timing must not be used for GO/NO-GO.
