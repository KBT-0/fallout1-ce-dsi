# Run 1 report — PROJECT_v3 §34

Measured:
- GitHub Actions run `31626859493` compiled and linked the 111-unit ARM9 feasibility closure with devkitARM r68, GCC 16.1.0, libnds 2.0.2, and Calico 1.2.0.
- Baseline ELF: text 902,600; data 247,036; bss 1,519,792; total 2,669,428 bytes.
- Optimized ELF: text 895,024; data 246,916; bss 1,518,904; total 2,660,844 bytes.
- Section GC saved 8,584 bytes against the baseline.
- Largest optimized core allocations include 480,000 bytes BSS in `light.cc`, 231,577 bytes BSS in `color.cc`, and 199,500 bytes writable data in `worldmap_walkmask.cc`.

Implemented:
- Added baseline and optimized ARM9 feasibility builds with exact ELF/map contracts.
- Expanded every devkitPro portlibs root correctly and isolated project library paths from `ds_rules` variables.
- Linked with the current supported Calico `ds9.specs` and `libcalico_ds9` startup runtime.
- Added and closed the checked-out SDL feasibility surface for all selected core sources.
- Added Phase 0C GNW current/peak byte and block instrumentation plus checkpoint and gap design.
- Added Phase 0D CPU/2D/3D/dirty/hybrid/dual-screen benchmark and VRAM layout design.
- Made CI preserve logs/maps/ELFs on failure and enforce successful baseline/optimized output evidence.

Built:
- `build/fallout1-dsi-feasibility-baseline.elf`
- `build/fallout1-dsi-feasibility-baseline.map`
- `build/fallout1-dsi-feasibility.elf`
- `build/fallout1-dsi-feasibility.map`

Tested:
- All 110 real Fallout/fpattern core translation units plus the SDL shim compile for ARMv5TE.
- Both ARM9 configurations link with libnds, NDS zlib, Calico, libm, and the DS9 startup runtime.
- Local host closure compile/link, shell syntax, whitespace, and exact Make dry-run checks pass.
- GitHub Actions compile, link, artifact upload, and output enforcement pass.

Unknown:
- Representative real-gameplay peak working set, largest free block, fragmentation, and retained allocations.
- Production DSi backend code/static/heap deltas.
- Real DSi VRAM remap/upload/render timing and sustainable p95/worst FPS.
- Real DSi SD throughput/latency and audio CPU/RAM cost.

Current feasibility status:
UNKNOWN — Phase 0A compile/link passes; Phase 0C and Phase 0D still control the GO/NO-GO decision.

Current blocker:
No software/CI blocker. Authoritative Phase 0D timing ultimately requires physical Nintendo DSi hardware.

Next highest-value action:
Implement the unified `dsi-bench.nds` runtime so Phase 0D render timing and Phase 0C heap checkpoints are produced by one hardware-testable artifact.

Hardware test required:
Yes for authoritative Phase 0D timing and final Phase 0C environment measurements; emulator timing is invalid for GO/NO-GO.
