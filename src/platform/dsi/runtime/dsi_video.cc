#include "dsi_video.h"

#include "dsi_runtime.h"

#include <nds.h>

#include <algorithm>
#include <cstring>

namespace fallout {
namespace {

struct DirtyRect {
    int left;
    int top;
    int right;
    int bottom;
    bool valid;
};

int gSourceWidth;
int gSourceHeight;
int gBackground = -1;
u16* gScreen;
u16 gPalette[256];
DirtyRect gDirty;
DsiRenderStrategy gStrategy = DsiRenderStrategy::Dirty2D;

void makeFullDirty()
{
    gDirty = { 0, 0, gSourceWidth, gSourceHeight, true };
}

} // namespace

bool dsiVideoInit(int sourceWidth, int sourceHeight)
{
    gSourceWidth = sourceWidth;
    gSourceHeight = sourceHeight;
    videoSetMode(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    gBackground = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    gScreen = static_cast<u16*>(bgGetGfxPtr(gBackground));
    if (gScreen == nullptr) return false;
    std::fill(gScreen, gScreen + 256 * 256, RGB15(0, 0, 0) | BIT(15));
    for (int index = 0; index < 256; ++index)
        gPalette[index] = RGB15(index >> 3, index >> 3, index >> 3) | BIT(15);
    makeFullDirty();
    dsiLog("VIDEO.SOURCE=%dx%d\n", sourceWidth, sourceHeight);
    dsiLog("VIDEO.OUTPUT=256x192_RGB15\n");
    dsiLog("VIDEO.STRATEGY=DIRTY_2D_PROVISIONAL\n");
    return true;
}

void dsiVideoExit()
{
    gScreen = nullptr;
    gBackground = -1;
}

void dsiVideoSetPalette(const SDL_Color* colors, int first, int count)
{
    if (colors == nullptr || first < 0 || count < 0 || first + count > 256) return;
    for (int index = 0; index < count; ++index) {
        const SDL_Color& color = colors[index];
        gPalette[first + index] = RGB15(color.r >> 3, color.g >> 3, color.b >> 3) | BIT(15);
    }
    makeFullDirty();
}

void dsiVideoMarkDirty(int x, int y, int width, int height)
{
    if (width <= 0 || height <= 0) return;
    const int left = std::max(0, x);
    const int top = std::max(0, y);
    const int right = std::min(gSourceWidth, x + width);
    const int bottom = std::min(gSourceHeight, y + height);
    if (left >= right || top >= bottom) return;
    if (!gDirty.valid) {
        gDirty = { left, top, right, bottom, true };
    } else {
        gDirty.left = std::min(gDirty.left, left);
        gDirty.top = std::min(gDirty.top, top);
        gDirty.right = std::max(gDirty.right, right);
        gDirty.bottom = std::max(gDirty.bottom, bottom);
    }
}

void dsiVideoPresent(const SDL_Surface* surface)
{
    if (surface == nullptr || surface->pixels == nullptr || gScreen == nullptr) return;
    if (!gDirty.valid) return;

    int outputLeft = gDirty.left * 256 / gSourceWidth;
    int outputTop = gDirty.top * 192 / gSourceHeight;
    int outputRight = (gDirty.right * 256 + gSourceWidth - 1) / gSourceWidth;
    int outputBottom = (gDirty.bottom * 192 + gSourceHeight - 1) / gSourceHeight;
    if (gStrategy == DsiRenderStrategy::CpuFull) {
        outputLeft = 0;
        outputTop = 0;
        outputRight = 256;
        outputBottom = 192;
    }
    outputLeft = std::max(0, outputLeft);
    outputTop = std::max(0, outputTop);
    outputRight = std::min(256, outputRight);
    outputBottom = std::min(192, outputBottom);
    const auto* source = static_cast<const unsigned char*>(surface->pixels);
    for (int y = outputTop; y < outputBottom; ++y) {
        const int sourceY = y * gSourceHeight / 192;
        u16* destination = gScreen + y * 256;
        const unsigned char* row = source + sourceY * surface->pitch;
        for (int x = outputLeft; x < outputRight; ++x) {
            const int sourceX = x * gSourceWidth / 256;
            destination[x] = gPalette[row[sourceX]];
        }
    }
    gDirty.valid = false;
    swiWaitForVBlank();
}

void dsiVideoSetStrategy(DsiRenderStrategy strategy)
{
    gStrategy = strategy;
    makeFullDirty();
}

DsiRenderStrategy dsiVideoStrategy()
{
    return gStrategy;
}

void dsiVideoMapTouch(int screenX, int screenY, int* sourceX, int* sourceY)
{
    if (sourceX != nullptr)
        *sourceX = std::clamp(screenX * gSourceWidth / 256, 0, gSourceWidth - 1);
    if (sourceY != nullptr)
        *sourceY = std::clamp(screenY * gSourceHeight / 192, 0, gSourceHeight - 1);
}

} // namespace fallout
