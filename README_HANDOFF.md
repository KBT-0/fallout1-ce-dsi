# Fallout 1 CE DSi — Phase 0 Run 1 fix-2 handoff

This package fixes the previous harness-only CI handoff.

## Critical new deliverable

`Makefile.dsi` now contains a real `feasibility` target and compiles the Fallout core source-directory set used by the 3DS port, replacing `src/platform/ctr` with a minimal DSi/SDL Phase-0 shim.

## Read first

1. `PATCH_APPLY.md`
2. `docs/RUN1_REPORT.md`
3. `docs/FEASIBILITY.md`
4. `docs/SIZE_REPORT.md`
5. `docs/CTR_AUDIT.md`
6. `docs/SDL_AUDIT.md`

## Intended outcome of the next CI run

Either:

```text
A. real ARM9 feasibility ELF + linker map + size output
```

or:

```text
B. the first real ARMv5TE compiler/linker errors from Fallout CE
```

Both are useful. `Makefile.dsi is missing` is no longer an expected outcome.
