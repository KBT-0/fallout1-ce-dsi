# DSi native SDL compatibility layer

This directory began as the PROJECT_v3 Phase 0A link shim. The compatibility
API is now backed by the native DSi runtime for timing, indexed
palette/framebuffer presentation, touch, buttons, and silent audio fallback.

Purpose:

1. keep `<SDL.h>` references independent of desktop/3DS SDL2;
2. retain the accepted Phase 0A lower-bound source contract;
3. provide the SDL-facing half of the production DSi backend.

Any numeric result produced while this shim is linked must be labeled:

```text
LOWER BOUND
```

The `fallout1-dsi.nds` build enables the engine's `__DSI__` paths and pairs
this API with `src/platform/dsi/runtime/`.
