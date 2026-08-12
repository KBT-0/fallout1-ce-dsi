#pragma once

#include <cstdint>

using u16 = std::uint16_t;

constexpr std::uint32_t KEY_A = 1u << 0;
constexpr std::uint32_t KEY_X = 1u << 1;
constexpr int MODE_5_2D = 0;
constexpr int VRAM_A_MAIN_BG = 0;
constexpr int BgType_Bmp16 = 0;
constexpr int BgSize_B16_256x256 = 0;

#define BIT(n) (1u << (n))
#define RGB15(r, g, b) \
    ((u16)(((r) & 31) | (((g) & 31) << 5) | (((b) & 31) << 10)))

void swiWaitForVBlank();
void scanKeys();
std::uint32_t keysHeld();
std::uint32_t keysDown();
bool pmMainLoop();
void consoleClear();
int iprintf(const char*, ...);
void videoSetMode(int);
void vramSetBankA(int);
int bgInit(int, int, int, int, int);
void* bgGetGfxPtr(int);
std::uint32_t timerTicks2usec(std::uint32_t);
