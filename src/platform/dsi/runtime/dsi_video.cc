#include "dsi_video.h"

#include "dsi_rectmap.h"
#include "dsi_runtime.h"

#include <nds.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace fallout {
namespace {

constexpr int kScreenWidth = 256;
constexpr int kScreenHeight = 192;
constexpr int kBitmapStride = 256;
constexpr size_t kBitmapBytes = 256 * 256;

struct DirtyRect {
    int left;
    int top;
    int right;
    int bottom;
    bool valid;
};

int gSourceWidth;
int gSourceHeight;
int gMainBackground = -1;
int gSubBackground = -1;
uint8_t* gMainScreen;
uint8_t* gSubScreen;
uint8_t* gMainStage;
uint8_t* gSubStage;
u16 gPalette[256];
bool gPaletteDirty;
bool gBottomFullDirty;
DirtyRect gDirty;
DsiRenderStrategy gStrategy = DsiRenderStrategy::Dirty2D;

void makeFullDirty()
{
    gDirty = { 0, 0, gSourceWidth, gSourceHeight, true };
    gBottomFullDirty = true;
}

DirtyRect outputDirty(const DirtyRect& sourceDirty,
    const DsiRectmapView& view, bool forceFull)
{
    if (forceFull) {
        return {
            view.destinationX,
            view.destinationY,
            view.destinationX + view.destinationWidth,
            view.destinationY + view.destinationHeight,
            true,
        };
    }
    if (!sourceDirty.valid) return { 0, 0, 0, 0, false };
    const int left = std::max(sourceDirty.left, view.sourceX);
    const int top = std::max(sourceDirty.top, view.sourceY);
    const int right = std::min(sourceDirty.right, view.sourceX + view.sourceWidth);
    const int bottom = std::min(sourceDirty.bottom, view.sourceY + view.sourceHeight);
    if (left >= right || top >= bottom) return { 0, 0, 0, 0, false };
    return {
        view.destinationX
            + (left - view.sourceX) * view.destinationWidth / view.sourceWidth,
        view.destinationY
            + (top - view.sourceY) * view.destinationHeight / view.sourceHeight,
        view.destinationX
            + ((right - view.sourceX) * view.destinationWidth
                  + view.sourceWidth - 1)
                / view.sourceWidth,
        view.destinationY
            + ((bottom - view.sourceY) * view.destinationHeight
                  + view.sourceHeight - 1)
                / view.sourceHeight,
        true,
    };
}

void stageView(const SDL_Surface* surface, const DsiRectmapView& view,
    DirtyRect dirty, uint8_t* stage)
{
    if (!dirty.valid) return;
    dirty.left = std::clamp(dirty.left, view.destinationX,
        view.destinationX + view.destinationWidth);
    dirty.top = std::clamp(dirty.top, view.destinationY,
        view.destinationY + view.destinationHeight);
    dirty.right = std::clamp(dirty.right, view.destinationX,
        view.destinationX + view.destinationWidth);
    dirty.bottom = std::clamp(dirty.bottom, view.destinationY,
        view.destinationY + view.destinationHeight);
    const auto* source = static_cast<const uint8_t*>(surface->pixels);
    for (int y = dirty.top; y < dirty.bottom; ++y) {
        const int sourceY = view.sourceY
            + (y - view.destinationY) * view.sourceHeight / view.destinationHeight;
        const uint8_t* sourceRow = source + sourceY * surface->pitch;
        uint8_t* destination = stage + y * kBitmapStride;
        for (int x = dirty.left; x < dirty.right; ++x) {
            const int sourceX = view.sourceX
                + (x - view.destinationX) * view.sourceWidth / view.destinationWidth;
            destination[x] = sourceRow[sourceX];
        }
    }
}

void uploadDirty(uint8_t* stage, uint8_t* target, DirtyRect dirty)
{
    if (!dirty.valid) return;
    const int left = std::max(0, dirty.left & ~3);
    const int right = std::min(kScreenWidth, (dirty.right + 3) & ~3);
    const int top = std::max(0, dirty.top);
    const int bottom = std::min(kScreenHeight, dirty.bottom);
    const int width = right - left;
    if (width <= 0 || top >= bottom) return;
    for (int y = top; y < bottom; ++y) {
        uint8_t* source = stage + y * kBitmapStride + left;
        uint8_t* destination = target + y * kBitmapStride + left;
        DC_FlushRange(source, width);
        dmaCopyWords(0, source, destination, width);
    }
}

} // namespace

bool dsiVideoInit(int sourceWidth, int sourceHeight)
{
    gSourceWidth = sourceWidth;
    gSourceHeight = sourceHeight;
    gMainStage = static_cast<uint8_t*>(std::calloc(1, kBitmapBytes));
    gSubStage = static_cast<uint8_t*>(std::calloc(1, kBitmapBytes));
    if (gMainStage == nullptr || gSubStage == nullptr) {
        dsiVideoExit();
        return false;
    }

    lcdMainOnTop();
    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_5_2D);
    vramSetPrimaryBanks(VRAM_A_MAIN_BG_0x06000000, VRAM_B_LCD,
        VRAM_C_SUB_BG_0x06200000, VRAM_D_LCD);
    gMainBackground = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    gSubBackground = bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    if (gMainBackground < 0 || gSubBackground < 0) {
        dsiVideoExit();
        return false;
    }
    gMainScreen = reinterpret_cast<uint8_t*>(bgGetGfxPtr(gMainBackground));
    gSubScreen = reinterpret_cast<uint8_t*>(bgGetGfxPtr(gSubBackground));
    if (gMainScreen == nullptr || gSubScreen == nullptr) {
        dsiVideoExit();
        return false;
    }
    std::memset(gMainScreen, 0, kBitmapBytes);
    std::memset(gSubScreen, 0, kBitmapBytes);
    for (int index = 0; index < 256; ++index) {
        gPalette[index] = RGB15(index >> 3, index >> 3, index >> 3);
    }
    gPaletteDirty = true;
    dsiRectmapInit(sourceWidth, sourceHeight);
    makeFullDirty();
    bgUpdate();
    dsiLog("VIDEO.SOURCE=%dx%d_INDEXED8\n", sourceWidth, sourceHeight);
    dsiLog("VIDEO.OUTPUT=MAIN_256x192_BG8_SUB_256x192_BG8\n");
    dsiLog("VIDEO.STRATEGY=DIRTY_2D_RECTMAP\n");
    dsiLog("VIDEO.FULL_REDRAW_POLICY=ONLY_TRUE_FULL_DIRTY\n");
    dsiStartupStage("VIDEO_INIT_OK");
    dsiLogMemory("04_AFTER_INDEXED_FRAMEBUFFER_AND_RENDER_STAGING", false);
    return true;
}

void dsiVideoExit()
{
    std::free(gMainStage);
    std::free(gSubStage);
    gMainStage = nullptr;
    gSubStage = nullptr;
    gMainScreen = nullptr;
    gSubScreen = nullptr;
    gMainBackground = -1;
    gSubBackground = -1;
}

void dsiVideoSetPalette(const SDL_Color* colors, int first, int count)
{
    if (colors == nullptr || first < 0 || count < 0 || first + count > 256) return;
    for (int index = 0; index < count; ++index) {
        const SDL_Color& color = colors[index];
        gPalette[first + index] = RGB15(color.r >> 3, color.g >> 3, color.b >> 3);
    }
    // Indexed VRAM retains pixel indices, so a palette fade never rebuilds or
    // uploads the 640x480 source.
    gPaletteDirty = true;
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
    if (surface == nullptr || surface->pixels == nullptr
        || gMainScreen == nullptr || gSubScreen == nullptr) {
        return;
    }
    if (!gDirty.valid && !gPaletteDirty && !gBottomFullDirty) return;

    const DsiRectmapView mainView = {
        0, 0, gSourceWidth, gSourceHeight, 0, 0, kScreenWidth, kScreenHeight,
    };
    const DsiRectmapView subView = dsiRectmapBottomView();
    DirtyRect mainDirty = outputDirty(gDirty, mainView,
        gStrategy == DsiRenderStrategy::CpuFull);
    DirtyRect subDirty = outputDirty(gDirty, subView,
        gBottomFullDirty || gStrategy == DsiRenderStrategy::CpuFull);
    DirtyRect subUploadDirty = subDirty;
    if (gBottomFullDirty) {
        std::memset(gSubStage, 0, kBitmapBytes);
        subUploadDirty = { 0, 0, kScreenWidth, kScreenHeight, true };
    }
    stageView(surface, mainView, mainDirty, gMainStage);
    stageView(surface, subView, subDirty, gSubStage);

    swiWaitForVBlank();
    if (gPaletteDirty) {
        DC_FlushRange(gPalette, sizeof(gPalette));
        dmaCopyWords(0, gPalette, BG_PALETTE, sizeof(gPalette));
        dmaCopyWords(0, gPalette, BG_PALETTE_SUB, sizeof(gPalette));
    }
    uploadDirty(gMainStage, gMainScreen, mainDirty);
    uploadDirty(gSubStage, gSubScreen, subUploadDirty);
    bgUpdate();

    gDirty.valid = false;
    gPaletteDirty = false;
    gBottomFullDirty = false;
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
    dsiRectmapMapTouch(screenX, screenY, sourceX, sourceY);
}

void dsiVideoRectmapChanged()
{
    gBottomFullDirty = true;
}

} // namespace fallout
