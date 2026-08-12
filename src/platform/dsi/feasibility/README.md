# DSi feasibility shim

This directory exists only for PROJECT_v3 Phase 0A.

It is intentionally incomplete and **must not** become the production platform backend.

Purpose:

1. make `<SDL.h>` references compile without linking a desktop/3DS SDL2;
2. expose real ARMv5TE compiler/linker failures deeper in the Fallout CE core;
3. obtain a lower-bound ELF size and linker map.

Any numeric result produced while this shim is linked must be labeled:

```text
LOWER BOUND
```

The production port must replace this with real DSi graphics, input, audio, timing and filesystem implementations.
