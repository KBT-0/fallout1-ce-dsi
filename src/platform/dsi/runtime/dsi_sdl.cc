#include <SDL.h>

// Production SDL-compatible facade for Fallout CE on Nintendo DSi. The API is
// intentionally limited to the subset used by the engine.

#include "platform/dsi/runtime/dsi_runtime.h"
#include "platform/dsi/runtime/dsi_video.h"

#include <nds.h>

#include <ctype.h>
#include <stdio.h>

struct SDL_Window { int unused; };
struct SDL_Renderer { int unused; };
struct SDL_Texture { Uint32 format; int w; int h; };
struct SDL_AudioStream { int unused; };
struct SDL_Cursor { int unused; };

static Uint32 gInitFlags = 0;
static int gRelativeMouseX = 0;
static int gRelativeMouseY = 0;
static int gVirtualMouseX = 320;
static int gVirtualMouseY = 240;
static Uint32 gMouseButtons = 0;
static bool gClockStarted = false;
static Uint32 gLastClockTicks = 0;
static Uint64 gAccumulatedClockTicks = 0;
static SDL_Event gEvents[24];
static unsigned int gEventRead = 0;
static unsigned int gEventWrite = 0;

static void pushEvent(const SDL_Event& event)
{
    const unsigned int next = (gEventWrite + 1) % 24;
    if (next != gEventRead) {
        gEvents[gEventWrite] = event;
        gEventWrite = next;
    }
}

static void pushKey(Uint32 type, Uint8 state, SDL_Scancode scancode)
{
    SDL_Event event = {};
    event.key.type = type;
    event.key.timestamp = SDL_GetTicks();
    event.key.state = state;
    event.key.keysym.scancode = scancode;
    pushEvent(event);
}

static void pumpNativeInput()
{
    scanKeys();
    const Uint32 down = keysDown();
    const Uint32 up = keysUp();
    const Uint32 held = keysHeld();
    const struct Mapping { Uint32 key; SDL_Scancode scancode; } mappings[] = {
        { KEY_A, SDL_SCANCODE_RETURN }, { KEY_B, SDL_SCANCODE_ESCAPE },
        { KEY_X, SDL_SCANCODE_SPACE }, { KEY_Y, SDL_SCANCODE_I },
        { KEY_START, SDL_SCANCODE_ESCAPE }, { KEY_SELECT, SDL_SCANCODE_TAB },
        { KEY_UP, SDL_SCANCODE_UP }, { KEY_DOWN, SDL_SCANCODE_DOWN },
        { KEY_LEFT, SDL_SCANCODE_LEFT }, { KEY_RIGHT, SDL_SCANCODE_RIGHT },
    };
    for (const Mapping& mapping : mappings) {
        if ((down & mapping.key) != 0)
            pushKey(SDL_KEYDOWN, SDL_PRESSED, mapping.scancode);
        if ((up & mapping.key) != 0)
            pushKey(SDL_KEYUP, SDL_RELEASED, mapping.scancode);
    }

    gMouseButtons = 0;
    if ((held & KEY_R) != 0) gMouseButtons |= SDL_BUTTON(SDL_BUTTON_LEFT);
    if ((held & KEY_L) != 0) gMouseButtons |= SDL_BUTTON(SDL_BUTTON_RIGHT);

    const bool touchHeld = (held & KEY_TOUCH) != 0;
    if (touchHeld) {
        touchPosition touch = {};
        touchRead(&touch);
        int sourceX = 0;
        int sourceY = 0;
        fallout::dsiVideoMapTouch(touch.px, touch.py, &sourceX, &sourceY);
        gRelativeMouseX += sourceX - gVirtualMouseX;
        gRelativeMouseY += sourceY - gVirtualMouseY;
        gVirtualMouseX = sourceX;
        gVirtualMouseY = sourceY;
        // Direct touch is the primary left mouse input. Feeding both GNW's
        // generic gesture queue and the rectmap-relative mouse path would move
        // the cursor twice using two different coordinate spaces.
        gMouseButtons |= SDL_BUTTON(SDL_BUTTON_LEFT);
    }

    if (!pmMainLoop()) {
        SDL_Event event = {};
        event.type = SDL_QUIT;
        pushEvent(event);
    }
}

static void* stubAlloc(size_t size)
{
    if (size == 0) {
        size = 1;
    }
    return calloc(1, size);
}

extern "C" {

int SDL_Init(Uint32 flags)
{
    return SDL_InitSubSystem(flags);
}

void SDL_Quit(void)
{
    gInitFlags = 0;
}

int SDL_InitSubSystem(Uint32 flags)
{
    gInitFlags |= flags;
    return 0;
}

void SDL_QuitSubSystem(Uint32 flags)
{
    gInitFlags &= ~flags;
}

Uint32 SDL_WasInit(Uint32 flags)
{
    return gInitFlags & flags;
}

int SDL_SetHint(const char*, const char*) { return SDL_TRUE; }
int SDL_ShowCursor(int) { return 0; }

SDL_Window* SDL_CreateWindow(const char*, int, int, int, int, Uint32)
{
    return static_cast<SDL_Window*>(stubAlloc(sizeof(SDL_Window)));
}

void SDL_DestroyWindow(SDL_Window* window) { free(window); }

SDL_Renderer* SDL_CreateRenderer(SDL_Window*, int, Uint32)
{
    return static_cast<SDL_Renderer*>(stubAlloc(sizeof(SDL_Renderer)));
}

void SDL_DestroyRenderer(SDL_Renderer* renderer) { free(renderer); }
int SDL_RenderSetLogicalSize(SDL_Renderer*, int, int) { return 0; }
int SDL_RenderClear(SDL_Renderer*) { return 0; }
int SDL_RenderCopy(SDL_Renderer*, SDL_Texture*, const SDL_Rect*, const SDL_Rect*) { return 0; }
void SDL_RenderPresent(SDL_Renderer*) { }

SDL_Texture* SDL_CreateTexture(SDL_Renderer*, Uint32 format, int, int w, int h)
{
    SDL_Texture* texture = static_cast<SDL_Texture*>(stubAlloc(sizeof(SDL_Texture)));
    if (texture != NULL) {
        texture->format = format;
        texture->w = w;
        texture->h = h;
    }
    return texture;
}

void SDL_DestroyTexture(SDL_Texture* texture) { free(texture); }

int SDL_QueryTexture(SDL_Texture* texture, Uint32* format, int* access, int* w, int* h)
{
    if (texture == NULL) return -1;
    if (format != NULL) *format = texture->format;
    if (access != NULL) *access = SDL_TEXTUREACCESS_STREAMING;
    if (w != NULL) *w = texture->w;
    if (h != NULL) *h = texture->h;
    return 0;
}

int SDL_UpdateTexture(SDL_Texture*, const SDL_Rect*, const void*, int) { return 0; }

static SDL_Surface* allocSurface(int width, int height, int depth, Uint32 format)
{
    SDL_Surface* surface = static_cast<SDL_Surface*>(stubAlloc(sizeof(SDL_Surface)));
    if (surface == NULL) return NULL;

    surface->format = static_cast<SDL_PixelFormat*>(stubAlloc(sizeof(SDL_PixelFormat)));
    if (surface->format == NULL) {
        free(surface);
        return NULL;
    }

    surface->format->format = format;
    if (depth == 8) {
        surface->format->palette = static_cast<SDL_Palette*>(stubAlloc(sizeof(SDL_Palette)));
        if (surface->format->palette != NULL) {
            surface->format->palette->ncolors = 256;
            surface->format->palette->colors = static_cast<SDL_Color*>(stubAlloc(sizeof(SDL_Color) * 256));
            surface->ownsPalette = 1;
        }
    }

    surface->w = width;
    surface->h = height;
    int bytesPerPixel = depth <= 8 ? 1 : (depth + 7) / 8;
    surface->pitch = width * bytesPerPixel;
    surface->pixels = stubAlloc(static_cast<size_t>(surface->pitch) * height);
    return surface;
}

SDL_Surface* SDL_CreateRGBSurface(Uint32, int width, int height, int depth,
    Uint32, Uint32, Uint32, Uint32)
{
    return allocSurface(width, height, depth, depth == 8 ? 0 : SDL_PIXELFORMAT_RGB888);
}

SDL_Surface* SDL_CreateRGBSurfaceWithFormat(Uint32, int width, int height, int depth, Uint32 format)
{
    return allocSurface(width, height, depth, format);
}

void SDL_FreeSurface(SDL_Surface* surface)
{
    if (surface == NULL) return;
    if (surface->format != NULL) {
        if (surface->ownsPalette && surface->format->palette != NULL) {
            free(surface->format->palette->colors);
            free(surface->format->palette);
        }
        free(surface->format);
    }
    free(surface->pixels);
    free(surface);
}

int SDL_SetPaletteColors(SDL_Palette* palette, const SDL_Color* colors, int firstcolor, int ncolors)
{
    if (palette == NULL || palette->colors == NULL || colors == NULL) return -1;
    if (firstcolor < 0 || ncolors < 0 || firstcolor + ncolors > palette->ncolors) return -1;
    memcpy(palette->colors + firstcolor, colors, static_cast<size_t>(ncolors) * sizeof(SDL_Color));
    fallout::dsiVideoSetPalette(colors, firstcolor, ncolors);
    return 0;
}

int SDL_SetSurfacePalette(SDL_Surface* surface, SDL_Palette* palette)
{
    if (surface == NULL || surface->format == NULL) return -1;
    if (surface->format->palette == palette) return 0;
    if (surface->ownsPalette && surface->format->palette != NULL && surface->format->palette != palette) {
        free(surface->format->palette->colors);
        free(surface->format->palette);
    }
    surface->format->palette = palette;
    surface->ownsPalette = 0;
    return 0;
}

int SDL_BlitSurface(SDL_Surface* src, const SDL_Rect* sourceRect,
    SDL_Surface* dst, SDL_Rect* destinationRect)
{
    if (src == NULL || dst == NULL || src->pixels == NULL || dst->pixels == NULL)
        return -1;
    SDL_Rect source = sourceRect != NULL ? *sourceRect : SDL_Rect { 0, 0, src->w, src->h };
    SDL_Rect destination = destinationRect != NULL
        ? *destinationRect
        : SDL_Rect { 0, 0, source.w, source.h };
    const int width = source.w < destination.w ? source.w : destination.w;
    const int height = source.h < destination.h ? source.h : destination.h;
    const int srcBytes = src->w > 0 ? src->pitch / src->w : 1;
    const int dstBytes = dst->w > 0 ? dst->pitch / dst->w : 1;
    if (srcBytes != dstBytes) return -1;
    for (int row = 0; row < height; ++row) {
        const Uint8* sourceRow = static_cast<const Uint8*>(src->pixels)
            + (source.y + row) * src->pitch + source.x * srcBytes;
        Uint8* destinationRow = static_cast<Uint8*>(dst->pixels)
            + (destination.y + row) * dst->pitch + destination.x * dstBytes;
        memcpy(destinationRow, sourceRow, static_cast<size_t>(width) * srcBytes);
    }
    return 0;
}
int SDL_LockSurface(SDL_Surface*) { return 0; }
void SDL_UnlockSurface(SDL_Surface*) { }

int SDL_PollEvent(SDL_Event* event)
{
    if (event == NULL) return 0;
    if (gEventRead == gEventWrite) pumpNativeInput();
    if (gEventRead == gEventWrite) return 0;
    *event = gEvents[gEventRead];
    gEventRead = (gEventRead + 1) % 24;
    return 1;
}
void SDL_PumpEvents(void) { pumpNativeInput(); }
Uint32 SDL_GetRelativeMouseState(int* x, int* y)
{
    pumpNativeInput();
    if (x != NULL) *x = gRelativeMouseX;
    if (y != NULL) *y = gRelativeMouseY;
    gRelativeMouseX = 0;
    gRelativeMouseY = 0;
    return gMouseButtons;
}
int SDL_SetRelativeMouseMode(SDL_bool) { return 0; }
void SDL_FlushEvents(Uint32, Uint32) { }
void SDL_StartTextInput(void) { }
void SDL_StopTextInput(void) { }
SDL_Keymod SDL_GetModState(void) { return KMOD_NONE; }

Uint32 SDL_GetTicks(void)
{
    if (!gClockStarted) {
        cpuStartTiming(0);
        gClockStarted = true;
        gLastClockTicks = cpuGetTiming();
        return 0;
    }
    const Uint32 ticks = cpuGetTiming();
    gAccumulatedClockTicks += static_cast<Uint32>(ticks - gLastClockTicks);
    gLastClockTicks = ticks;
    return static_cast<Uint32>((gAccumulatedClockTicks * 1000u) / BUS_CLOCK);
}

void SDL_Delay(Uint32 ms)
{
    const Uint32 start = SDL_GetTicks();
    while (SDL_GetTicks() - start < ms && pmMainLoop()) swiWaitForVBlank();
}

SDL_TimerID SDL_AddTimer(Uint32, SDL_TimerCallback, void*) { return 0; }
SDL_bool SDL_RemoveTimer(SDL_TimerID) { return SDL_FALSE; }

SDL_AudioDeviceID SDL_OpenAudioDevice(const char*, int, const SDL_AudioSpec* desired,
    SDL_AudioSpec* obtained, int)
{
    if (obtained != NULL && desired != NULL) *obtained = *desired;
    return 0;
}

void SDL_CloseAudioDevice(SDL_AudioDeviceID) { }
void SDL_PauseAudioDevice(SDL_AudioDeviceID, int) { }

SDL_AudioStream* SDL_NewAudioStream(SDL_AudioFormat, Uint8, int, SDL_AudioFormat, Uint8, int)
{
    return static_cast<SDL_AudioStream*>(stubAlloc(sizeof(SDL_AudioStream)));
}

void SDL_FreeAudioStream(SDL_AudioStream* stream) { free(stream); }
int SDL_AudioStreamPut(SDL_AudioStream*, const void*, int) { return 0; }
int SDL_AudioStreamGet(SDL_AudioStream*, void* buf, int len)
{
    if (buf != NULL && len > 0) memset(buf, 0, static_cast<size_t>(len));
    return len;
}
int SDL_AudioStreamAvailable(SDL_AudioStream*) { return 0; }

void SDL_MixAudioFormat(Uint8* dst, const Uint8* src, SDL_AudioFormat, Uint32 len, int)
{
    if (dst != NULL && src != NULL && len != 0) memcpy(dst, src, len);
}

static int fold(int c) { return tolower(static_cast<unsigned char>(c)); }

int SDL_strcasecmp(const char* a, const char* b)
{
    if (a == NULL || b == NULL) return a == b ? 0 : (a == NULL ? -1 : 1);
    while (*a != '\0' && *b != '\0') {
        int da = fold(*a++);
        int db = fold(*b++);
        if (da != db) return da - db;
    }
    return fold(*a) - fold(*b);
}

int SDL_strncasecmp(const char* a, const char* b, size_t n)
{
    if (n == 0) return 0;
    if (a == NULL || b == NULL) return a == b ? 0 : (a == NULL ? -1 : 1);
    for (size_t i = 0; i < n; ++i) {
        int da = fold(a[i]);
        int db = fold(b[i]);
        if (da != db) return da - db;
        if (a[i] == '\0' || b[i] == '\0') return 0;
    }
    return 0;
}

char* SDL_strupr(char* s)
{
    if (s != NULL) for (char* p = s; *p != '\0'; ++p) *p = static_cast<char>(toupper(static_cast<unsigned char>(*p)));
    return s;
}

char* SDL_strlwr(char* s)
{
    if (s != NULL) for (char* p = s; *p != '\0'; ++p) *p = static_cast<char>(tolower(static_cast<unsigned char>(*p)));
    return s;
}

char* SDL_itoa(int value, char* str, int radix)
{
    if (str == NULL) return NULL;
    if (radix == 16) snprintf(str, 34, "%x", value);
    else if (radix == 8) snprintf(str, 34, "%o", value);
    else snprintf(str, 34, "%d", value);
    return str;
}

char* SDL_strdup(const char* s)
{
    if (s == NULL) return NULL;
    size_t n = strlen(s) + 1;
    char* copy = static_cast<char*>(malloc(n));
    if (copy != NULL) memcpy(copy, s, n);
    return copy;
}

char* SDL_GetBasePath(void) { return SDL_strdup("sd:/fallout1/original/"); }
const char* SDL_AndroidGetExternalStoragePath(void) { return "sd:/fallout1/original/"; }
void SDL_free(void* p) { free(p); }
void* SDL_malloc(size_t size) { return malloc(size); }
const char* SDL_GetError(void) { return "DSi native SDL compatibility layer"; }
void SDL_SetWindowTitle(SDL_Window*, const char*) { }
SDL_Cursor* SDL_GetCursor(void) { return NULL; }
SDL_Cursor* SDL_CreateSystemCursor(int)
{
    return static_cast<SDL_Cursor*>(stubAlloc(sizeof(SDL_Cursor)));
}
void SDL_SetCursor(SDL_Cursor*) { }
void SDL_FreeCursor(SDL_Cursor* cursor) { free(cursor); }
int SDL_ShowSimpleMessageBox(Uint32, const char*, const char* message, SDL_Window*)
{
    fallout::dsiFatal(message);
    return 0;
}
void SDL_LogMessageV(int, int, const char* format, va_list args)
{
    fallout::dsiLogV(format, args);
}

} // extern "C"
