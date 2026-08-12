#ifndef FALLOUT_DSI_FEASIBILITY_SDL_H_
#define FALLOUT_DSI_FEASIBILITY_SDL_H_

// Minimal SDL2-shaped compile/link shim for PROJECT_v3 Phase 0A.
// THIS IS NOT A PRODUCTION SDL IMPLEMENTATION AND IS NOT RUNTIME-SAFE FOR THE
// GAME. Its only purpose is to let the real Fallout engine reach deeper ARM9
// compile/link stages so resident code size and missing-platform blockers can
// be measured.

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t Uint8;
typedef int8_t Sint8;
typedef uint16_t Uint16;
typedef int16_t Sint16;
typedef uint32_t Uint32;
typedef int32_t Sint32;
typedef uint64_t Uint64;
typedef int64_t Sint64;
typedef int64_t SDL_FingerID;
typedef Uint16 SDL_AudioFormat;
typedef Uint32 SDL_AudioDeviceID;
typedef int SDL_TimerID;
typedef int SDL_bool;
typedef Uint16 SDL_Keymod;

#define SDL_FALSE 0
#define SDL_TRUE 1

#define SDL_DISABLE 0
#define SDL_ENABLE 1

#define SDL_INIT_TIMER  0x00000001u
#define SDL_INIT_AUDIO  0x00000010u
#define SDL_INIT_VIDEO  0x00000020u
#define SDL_INIT_EVENTS 0x00004000u

#define SDL_WINDOW_FULLSCREEN    0x00000001u
#define SDL_WINDOW_OPENGL        0x00000002u
#define SDL_WINDOW_ALLOW_HIGHDPI 0x00002000u
#define SDL_WINDOWPOS_UNDEFINED  0

#define SDL_TEXTUREACCESS_STREAMING 1
#define SDL_PIXELFORMAT_RGB888 0x16161804u
#define SDL_BITSPERPIXEL(X) 32

#define SDL_HINT_RENDER_DRIVER "SDL_RENDER_DRIVER"
#define SDL_HINT_MOUSE_TOUCH_EVENTS "SDL_MOUSE_TOUCH_EVENTS"
#define SDL_HINT_TOUCH_MOUSE_EVENTS "SDL_TOUCH_MOUSE_EVENTS"

#define SDL_PRESSED 1
#define SDL_RELEASED 0

#define SDL_BUTTON_LEFT 1
#define SDL_BUTTON_MIDDLE 2
#define SDL_BUTTON_RIGHT 3
#define SDL_BUTTON(X) (1u << ((X)-1))

#define AUDIO_S8 0x8008u
#define AUDIO_S16 0x8010u
#define AUDIO_S16SYS AUDIO_S16
#define SDL_AUDIO_ALLOW_ANY_CHANGE 0x0000000Fu
#define SDL_MIX_MAXVOLUME 128

#define SDL_VERSION_ATLEAST(X, Y, Z) 0

#define KMOD_NONE   0x0000u
#define KMOD_NUM    0x1000u
#define KMOD_CAPS   0x2000u
#define KMOD_SCROLL 0x8000u

#define SDL_LOG_CATEGORY_APPLICATION 0
#define SDL_LOG_PRIORITY_INFO 3
#define SDL_MESSAGEBOX_ERROR 0x00000010u
#define SDL_SYSTEM_CURSOR_ARROW 0

// SDL2 event IDs used by Fallout CE. Numeric identity is irrelevant to this
// compile/link-only shim; unique values are sufficient.
enum {
    SDL_FIRSTEVENT = 0,
    SDL_QUIT = 0x100,
    SDL_WINDOWEVENT = 0x200,
    SDL_KEYDOWN = 0x300,
    SDL_KEYUP,
    SDL_TEXTINPUT,
    SDL_MOUSEMOTION = 0x400,
    SDL_MOUSEBUTTONDOWN,
    SDL_MOUSEBUTTONUP,
    SDL_MOUSEWHEEL,
    SDL_FINGERDOWN = 0x700,
    SDL_FINGERUP,
    SDL_FINGERMOTION,
};

enum {
    SDL_WINDOWEVENT_NONE = 0,
    SDL_WINDOWEVENT_EXPOSED = 3,
    SDL_WINDOWEVENT_SIZE_CHANGED = 6,
    SDL_WINDOWEVENT_FOCUS_GAINED = 12,
    SDL_WINDOWEVENT_FOCUS_LOST = 13,
};

// Minimal SDL2 scancode namespace used by Fallout's original keyboard map.
// Keep values below SDL_NUM_SCANCODES so array indexing remains well formed.
typedef enum SDL_Scancode {
    SDL_SCANCODE_UNKNOWN = 0,
    SDL_SCANCODE_A,
    SDL_SCANCODE_B,
    SDL_SCANCODE_C,
    SDL_SCANCODE_D,
    SDL_SCANCODE_E,
    SDL_SCANCODE_F,
    SDL_SCANCODE_G,
    SDL_SCANCODE_H,
    SDL_SCANCODE_I,
    SDL_SCANCODE_J,
    SDL_SCANCODE_K,
    SDL_SCANCODE_L,
    SDL_SCANCODE_M,
    SDL_SCANCODE_N,
    SDL_SCANCODE_O,
    SDL_SCANCODE_P,
    SDL_SCANCODE_Q,
    SDL_SCANCODE_R,
    SDL_SCANCODE_S,
    SDL_SCANCODE_T,
    SDL_SCANCODE_U,
    SDL_SCANCODE_V,
    SDL_SCANCODE_W,
    SDL_SCANCODE_X,
    SDL_SCANCODE_Y,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_4,
    SDL_SCANCODE_5,
    SDL_SCANCODE_6,
    SDL_SCANCODE_7,
    SDL_SCANCODE_8,
    SDL_SCANCODE_9,
    SDL_SCANCODE_0,
    SDL_SCANCODE_RETURN,
    SDL_SCANCODE_ESCAPE,
    SDL_SCANCODE_BACKSPACE,
    SDL_SCANCODE_TAB,
    SDL_SCANCODE_SPACE,
    SDL_SCANCODE_MINUS,
    SDL_SCANCODE_EQUALS,
    SDL_SCANCODE_LEFTBRACKET,
    SDL_SCANCODE_RIGHTBRACKET,
    SDL_SCANCODE_BACKSLASH,
    SDL_SCANCODE_SEMICOLON,
    SDL_SCANCODE_APOSTROPHE,
    SDL_SCANCODE_GRAVE,
    SDL_SCANCODE_COMMA,
    SDL_SCANCODE_PERIOD,
    SDL_SCANCODE_SLASH,
    SDL_SCANCODE_CAPSLOCK,
    SDL_SCANCODE_F1,
    SDL_SCANCODE_F2,
    SDL_SCANCODE_F3,
    SDL_SCANCODE_F4,
    SDL_SCANCODE_F5,
    SDL_SCANCODE_F6,
    SDL_SCANCODE_F7,
    SDL_SCANCODE_F8,
    SDL_SCANCODE_F9,
    SDL_SCANCODE_F10,
    SDL_SCANCODE_F11,
    SDL_SCANCODE_F12,
    SDL_SCANCODE_F13,
    SDL_SCANCODE_F14,
    SDL_SCANCODE_F15,
    SDL_SCANCODE_PRINTSCREEN,
    SDL_SCANCODE_SCROLLLOCK,
    SDL_SCANCODE_PAUSE,
    SDL_SCANCODE_INSERT,
    SDL_SCANCODE_HOME,
    SDL_SCANCODE_PRIOR,
    SDL_SCANCODE_DELETE,
    SDL_SCANCODE_END,
    SDL_SCANCODE_PAGEDOWN,
    SDL_SCANCODE_RIGHT,
    SDL_SCANCODE_LEFT,
    SDL_SCANCODE_DOWN,
    SDL_SCANCODE_UP,
    SDL_SCANCODE_NUMLOCKCLEAR,
    SDL_SCANCODE_KP_DIVIDE,
    SDL_SCANCODE_KP_MULTIPLY,
    SDL_SCANCODE_KP_MINUS,
    SDL_SCANCODE_KP_PLUS,
    SDL_SCANCODE_KP_ENTER,
    SDL_SCANCODE_KP_1,
    SDL_SCANCODE_KP_2,
    SDL_SCANCODE_KP_3,
    SDL_SCANCODE_KP_4,
    SDL_SCANCODE_KP_5,
    SDL_SCANCODE_KP_6,
    SDL_SCANCODE_KP_7,
    SDL_SCANCODE_KP_8,
    SDL_SCANCODE_KP_9,
    SDL_SCANCODE_KP_0,
    SDL_SCANCODE_KP_DECIMAL,
    SDL_SCANCODE_KP_EQUALS,
    SDL_SCANCODE_KP_COMMA,
    SDL_SCANCODE_APPLICATION,
    SDL_SCANCODE_STOP,
    SDL_SCANCODE_LCTRL,
    SDL_SCANCODE_LSHIFT,
    SDL_SCANCODE_LALT,
    SDL_SCANCODE_LGUI,
    SDL_SCANCODE_RCTRL,
    SDL_SCANCODE_RSHIFT,
    SDL_SCANCODE_RALT,
    SDL_SCANCODE_RGUI,
} SDL_Scancode;

#define SDL_NUM_SCANCODES 512

typedef struct SDL_Color {
    Uint8 r, g, b, a;
} SDL_Color;

typedef struct SDL_Rect {
    int x, y, w, h;
} SDL_Rect;

typedef struct SDL_Palette {
    int ncolors;
    SDL_Color* colors;
} SDL_Palette;

typedef struct SDL_PixelFormat {
    Uint32 format;
    SDL_Palette* palette;
} SDL_PixelFormat;

typedef struct SDL_Surface {
    Uint32 flags;
    SDL_PixelFormat* format;
    int w;
    int h;
    int pitch;
    void* pixels;
    int ownsPalette;
} SDL_Surface;

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_AudioStream SDL_AudioStream;
typedef struct SDL_Cursor SDL_Cursor;
typedef Uint32 (*SDL_TimerCallback)(Uint32 interval, void* param);

typedef struct SDL_Keysym {
    SDL_Scancode scancode;
    int sym;
    Uint16 mod;
} SDL_Keysym;

typedef struct SDL_KeyboardEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint8 state;
    Uint8 repeat;
    SDL_Keysym keysym;
} SDL_KeyboardEvent;

typedef struct SDL_MouseMotionEvent {
    Uint32 type;
    Uint32 timestamp;
    int x;
    int y;
    int xrel;
    int yrel;
} SDL_MouseMotionEvent;

typedef struct SDL_MouseButtonEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint8 button;
    Uint8 state;
    int x;
    int y;
} SDL_MouseButtonEvent;

typedef struct SDL_MouseWheelEvent {
    Uint32 type;
    Uint32 timestamp;
    int x;
    int y;
} SDL_MouseWheelEvent;

typedef struct SDL_TouchFingerEvent {
    Uint32 type;
    Uint32 timestamp;
    SDL_FingerID fingerId;
    float x;
    float y;
    float dx;
    float dy;
    float pressure;
} SDL_TouchFingerEvent;

typedef struct SDL_WindowEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint8 event;
    int data1;
    int data2;
} SDL_WindowEvent;

typedef union SDL_Event {
    Uint32 type;
    SDL_KeyboardEvent key;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
    SDL_MouseWheelEvent wheel;
    SDL_TouchFingerEvent tfinger;
    SDL_WindowEvent window;
    Uint8 padding[64];
} SDL_Event;

typedef void (*SDL_AudioCallback)(void* userdata, Uint8* stream, int len);

typedef struct SDL_AudioSpec {
    int freq;
    SDL_AudioFormat format;
    Uint8 channels;
    Uint8 silence;
    Uint16 samples;
    Uint16 padding;
    Uint32 size;
    SDL_AudioCallback callback;
    void* userdata;
} SDL_AudioSpec;

int SDL_Init(Uint32 flags);
void SDL_Quit(void);
int SDL_InitSubSystem(Uint32 flags);
void SDL_QuitSubSystem(Uint32 flags);
Uint32 SDL_WasInit(Uint32 flags);

int SDL_SetHint(const char* name, const char* value);
int SDL_ShowCursor(int toggle);

SDL_Window* SDL_CreateWindow(const char* title, int x, int y, int w, int h, Uint32 flags);
void SDL_DestroyWindow(SDL_Window* window);

SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, int index, Uint32 flags);
void SDL_DestroyRenderer(SDL_Renderer* renderer);
int SDL_RenderSetLogicalSize(SDL_Renderer* renderer, int w, int h);
int SDL_RenderClear(SDL_Renderer* renderer);
int SDL_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* src, const SDL_Rect* dst);
void SDL_RenderPresent(SDL_Renderer* renderer);

SDL_Texture* SDL_CreateTexture(SDL_Renderer* renderer, Uint32 format, int access, int w, int h);
void SDL_DestroyTexture(SDL_Texture* texture);
int SDL_QueryTexture(SDL_Texture* texture, Uint32* format, int* access, int* w, int* h);
int SDL_UpdateTexture(SDL_Texture* texture, const SDL_Rect* rect, const void* pixels, int pitch);

SDL_Surface* SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth,
    Uint32 rmask, Uint32 gmask, Uint32 bmask, Uint32 amask);
SDL_Surface* SDL_CreateRGBSurfaceWithFormat(Uint32 flags, int width, int height, int depth, Uint32 format);
void SDL_FreeSurface(SDL_Surface* surface);
int SDL_SetPaletteColors(SDL_Palette* palette, const SDL_Color* colors, int firstcolor, int ncolors);
int SDL_SetSurfacePalette(SDL_Surface* surface, SDL_Palette* palette);
int SDL_BlitSurface(SDL_Surface* src, const SDL_Rect* srcrect, SDL_Surface* dst, SDL_Rect* dstrect);
int SDL_LockSurface(SDL_Surface* surface);
void SDL_UnlockSurface(SDL_Surface* surface);

int SDL_PollEvent(SDL_Event* event);
void SDL_PumpEvents(void);
Uint32 SDL_GetRelativeMouseState(int* x, int* y);
int SDL_SetRelativeMouseMode(SDL_bool enabled);
void SDL_FlushEvents(Uint32 minType, Uint32 maxType);
void SDL_StartTextInput(void);
void SDL_StopTextInput(void);
SDL_Keymod SDL_GetModState(void);

Uint32 SDL_GetTicks(void);
void SDL_Delay(Uint32 ms);
SDL_TimerID SDL_AddTimer(Uint32 interval, SDL_TimerCallback callback, void* param);
SDL_bool SDL_RemoveTimer(SDL_TimerID id);

SDL_AudioDeviceID SDL_OpenAudioDevice(const char* device, int iscapture,
    const SDL_AudioSpec* desired, SDL_AudioSpec* obtained, int allowed_changes);
void SDL_CloseAudioDevice(SDL_AudioDeviceID dev);
void SDL_PauseAudioDevice(SDL_AudioDeviceID dev, int pause_on);
SDL_AudioStream* SDL_NewAudioStream(SDL_AudioFormat src_format, Uint8 src_channels, int src_rate,
    SDL_AudioFormat dst_format, Uint8 dst_channels, int dst_rate);
void SDL_FreeAudioStream(SDL_AudioStream* stream);
int SDL_AudioStreamPut(SDL_AudioStream* stream, const void* buf, int len);
int SDL_AudioStreamGet(SDL_AudioStream* stream, void* buf, int len);
int SDL_AudioStreamAvailable(SDL_AudioStream* stream);
void SDL_MixAudioFormat(Uint8* dst, const Uint8* src, SDL_AudioFormat format, Uint32 len, int volume);

int SDL_strcasecmp(const char* a, const char* b);
int SDL_strncasecmp(const char* a, const char* b, size_t n);
char* SDL_strupr(char* s);
char* SDL_strlwr(char* s);
char* SDL_itoa(int value, char* str, int radix);
char* SDL_strdup(const char* s);

char* SDL_GetBasePath(void);
const char* SDL_AndroidGetExternalStoragePath(void);
void SDL_free(void* p);
void* SDL_malloc(size_t size);
const char* SDL_GetError(void);
void SDL_SetWindowTitle(SDL_Window* window, const char* title);
SDL_Cursor* SDL_GetCursor(void);
SDL_Cursor* SDL_CreateSystemCursor(int id);
void SDL_SetCursor(SDL_Cursor* cursor);
void SDL_FreeCursor(SDL_Cursor* cursor);
int SDL_ShowSimpleMessageBox(Uint32 flags, const char* title, const char* message, SDL_Window* window);
void SDL_LogMessageV(int category, int priority, const char* format, va_list ap);

#define SDL_memcpy memcpy
#define SDL_memset memset

#ifdef __cplusplus
}
#endif

#endif
