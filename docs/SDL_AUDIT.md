# SDL_AUDIT — Run 1 checked-out-source inventory

## Status

```text
SOURCE-WIDE SCANNED; FEASIBILITY COMPILE/LINK CLOSED ON HOST
```

The checked-out source was scanned locally. The CI workflow also emits:

```text
build/ci/sdl-symbols.txt
build/ci/sdl-call-sites.txt
```

from the exact ARM9 CI checkout. All 111 feasibility translation units compile
and link with the host compiler and the shim; ARM9 compilation remains the
authoritative conditional-compilation check.

## Pinned upstream reference

Repository: `alexbatalov/fallout1-ce`
Commit: `0609bcfd0ec40ff0571d0f57fab2821eb461dc8b` (`Externalize adecode`)

The upstream CMake build uses C++17 and links SDL2.

## SDL usage observed by subsystem

### 1. Rendering / surfaces / palette

Observed in `src/plib/gnw/svga.cc`:

```text
SDL_Window
SDL_Surface
SDL_Renderer
SDL_Texture
SDL_Color
SDL_Rect

SDL_SetPaletteColors
SDL_BlitSurface
SDL_SetHint
SDL_InitSubSystem(SDL_INIT_VIDEO)
SDL_CreateWindow
SDL_CreateRGBSurface
SDL_DestroyWindow
SDL_QuitSubSystem
```

The remainder of the file also creates/updates/destroys a renderer/texture path for desktop presentation.

DSi disposition:

```text
SDL window/display/renderer/texture path -> REQUIRES NATIVE DSi REPLACEMENT
8-bit Fallout surface/palette semantics  -> PRESERVE OR SHIM MINIMALLY
palette update                           -> CUSTOM DSi PALETTE/VRAM PATH
blit semantics used by engine            -> CUSTOM SHIM OR ENGINE-SIDE WRAPPER
```

The DSi port should not attempt to reproduce desktop window creation.

### 2. Input / mouse / events

Observed in `src/plib/gnw/dxinput.cc`:

```text
SDL_InitSubSystem(SDL_INIT_EVENTS)
SDL_QuitSubSystem(SDL_INIT_EVENTS)
SDL_PumpEvents
SDL_GetRelativeMouseState
SDL_BUTTON(...)
SDL_BUTTON_LEFT
SDL_BUTTON_RIGHT
```

Additional input source files inspected in the web audit use SDL event/scancode/touch types and polling behavior, including:
- `SDL_Event`
- `SDL_PollEvent`
- keyboard scancodes / `SDL_NUM_SCANCODES`
- mouse button/motion/wheel events
- touch/finger events
- quit/window events
- pressed-state constants

DSi disposition:

```text
event pump / relative mouse -> CUSTOM DSi INPUT BACKEND
touch                      -> LIBNDS/CALICO TOUCH + RECTMAP COORDINATE MAP
keyboard/scancode           -> MINIMAL COMPAT TABLE FOR ACTUAL FALLOUT NEEDS
window events               -> REMOVE/NO-OP WHERE SAFE
```

### 3. Audio

Observed in `src/audio_engine.cc`:

```text
SDL_AudioStream
SDL_AudioSpec
SDL_AudioDeviceID
SDL_PauseAudioDevice
SDL_MIX_MAXVOLUME
SDL_NewAudioStream
SDL_FreeAudioStream
SDL_InitSubSystem(SDL_INIT_AUDIO)
SDL_OpenAudioDevice
SDL_CloseAudioDevice
SDL_WasInit
SDL_QuitSubSystem
SDL_AudioStreamPut / Get / Available
SDL_MixAudioFormat
```

The source also uses `std::recursive_mutex` around sound buffers.

DSi disposition:

```text
SDL audio device          -> REQUIRES NATIVE DSi AUDIO BACKEND
SDL_AudioStream resample  -> MUST BE REPLACED OR REIMPLEMENTED
mixing                    -> NATIVE/CUSTOM MIXER OR DSi-SUITABLE LIBRARY
recursive_mutex           -> THREADING AUDIT REQUIRED; avoid assuming full std::thread runtime
```

This is a significant porting surface, not a trivial stub.

### 4. Timing

Observed in `src/fps_limiter.cc`:

```text
SDL_GetTicks
SDL_Delay
```

DSi disposition:

```text
WRAP WITH DSi TIMER/VBLANK/TICK APIs
```

This should be a small compatibility surface compared with video/audio.

### 5. Platform/string compatibility

Observed in `src/platform_compat.cc`:

```text
SDL_strcasecmp
SDL_strncasecmp
SDL_strupr
SDL_strlwr
SDL_itoa
```

Other compatibility helpers use ordinary POSIX-style file/directory primitives on non-Windows systems.

DSi disposition:

```text
SDL string helpers -> REPLACE WITH SMALL LOCAL COMPAT FUNCTIONS
filesystem helpers -> AUDIT AGAINST FAT/DSi SD FILESYSTEM APIs
```

### 6. Window/app initialization

Observed in `winmain`-related code during the web audit:

```text
SDL_SetHint
SDL_GetBasePath / SDL_free on some host branches
SDL_ShowCursor
platform-specific Android/iOS path helpers on conditional branches
```

Many host-only branches may compile out on DSi, but the checked-out CI inventory must prove which symbols survive.

## Architectural conclusion

The minimum credible DSi strategy is **not "port all of SDL2" by default**.

Current best candidate:

```text
Fallout engine
  |
  +-- very small SDL-compatible types/helpers retained where cheap
  +-- native DSi graphics backend
  +-- native DSi input backend
  +-- native DSi timing wrapper
  +-- native DSi audio/mixer replacement
  +-- FAT/SD filesystem compatibility
```

Whether a maintained DS/DSi SDL implementation can reduce this work remains a verification task; none is assumed.

## Required ARM9 CI closure

The CI checkout must run a source-wide symbol/call-site scan. After it returns:

1. confirm which conditional symbols survive the ARM9 preprocessing path;
2. preserve `sdl-symbols.txt` and `sdl-call-sites.txt` with the build artifact;
3. classify any new ARM9-only dependency before extending the shim;
4. update the production-backend estimate after the native benchmark skeleton.

## Primary source URLs

```text
https://github.com/alexbatalov/fallout1-ce/commit/0609bcfd0ec40ff0571d0f57fab2821eb461dc8b
https://raw.githubusercontent.com/alexbatalov/fallout1-ce/refs/heads/main/src/audio_engine.cc
https://raw.githubusercontent.com/alexbatalov/fallout1-ce/refs/heads/main/src/plib/gnw/svga.cc
https://github.com/alexbatalov/fallout1-ce/blob/main/src/plib/gnw/dxinput.cc
https://github.com/alexbatalov/fallout1-ce/blob/main/src/fps_limiter.cc
https://raw.githubusercontent.com/alexbatalov/fallout1-ce/refs/heads/main/src/platform_compat.cc
```
