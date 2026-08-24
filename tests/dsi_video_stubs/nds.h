#pragma once

// Host-side stand-in for libnds, sized to exactly what the production DSi
// video backend touches. VRAM is modelled as plain buffers so the renderer's
// output can be compared byte for byte against an independent reference.

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;

#define RGB15(r, g, b) \
    ((u16)(((r) & 31) | (((g) & 31) << 5) | (((b) & 31) << 10)))

enum { MODE_5_2D = 0x10005 };
enum {
    VRAM_A_MAIN_BG_0x06000000,
    VRAM_B_LCD,
    VRAM_C_SUB_BG_0x06200000,
    VRAM_D_LCD,
};
enum { BgType_Bmp8 = 4 };
enum { BgSize_B8_256x256 = 1 };

extern u8 gHostMainVram[64 * 1024];
extern u8 gHostSubVram[64 * 1024];
extern u16 gHostMainPalette[256];
extern u16 gHostSubPalette[256];

#define BG_PALETTE (gHostMainPalette)
#define BG_PALETTE_SUB (gHostSubPalette)

inline void lcdMainOnTop() { }
inline void videoSetMode(int) { }
inline void videoSetModeSub(int) { }
inline u32 vramSetPrimaryBanks(int, int, int, int) { return 0; }
inline void bgUpdate() { }
inline void swiWaitForVBlank() { }
inline int bgInit(int, int, int, int, int) { return 3; }
inline int bgInitSub(int, int, int, int, int) { return 7; }
inline void* bgGetGfxPtr(int id) { return id == 3 ? gHostMainVram : gHostSubVram; }
inline void DC_FlushRange(const void*, std::size_t) { }
inline void dmaCopyWords(u8, const void* src, void* dest, u32 size)
{
    // libnds truncates to whole words; reproduce that so a bad byte count in
    // the caller shows up here instead of silently working.
    std::memcpy(dest, src, size & ~3u);
}
