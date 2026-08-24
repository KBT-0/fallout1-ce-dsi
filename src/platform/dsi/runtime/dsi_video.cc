#include "dsi_video.h"

#include "dsi_rectmap.h"
#include "dsi_runtime.h"

#include <nds.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <malloc.h>

namespace fallout {
namespace {

constexpr int kScreenWidth = 256;
constexpr int kScreenHeight = 192;
constexpr int kBitmapStride = 256;
constexpr size_t kVisibleBytes = kBitmapStride * kScreenHeight;
constexpr size_t kBitmapBytes = kBitmapStride * 256;
constexpr uint32_t kFnvOffset = 2166136261U;
constexpr uint32_t kFnvPrime = 16777619U;
constexpr unsigned int kPaletteLogLimit = 8;

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
alignas(4) u16 gPalette[256];
alignas(4) SDL_Color gSourcePalette[256];
bool gPaletteDirty;
bool gBottomFullDirty;
bool gSurfaceLogged;
bool gPaletteEntriesLogged;
bool gPaletteIsolationLogged;
bool gFrameLogPending;
unsigned int gPaletteUpdateSequence;
DirtyRect gDirty;
DsiRenderStrategy gStrategy = DsiRenderStrategy::Full2DReference;

const char* renderModeName()
{
    return gStrategy == DsiRenderStrategy::Full2DReference
        ? "FULL_2D_REFERENCE"
        : "DIRTY_2D";
}

void hashByte(uint32_t& hash, uint8_t value)
{
    hash = (hash ^ value) * kFnvPrime;
}

uint32_t hashBytes(const uint8_t* bytes, size_t size)
{
    uint32_t hash = kFnvOffset;
    for (size_t index = 0; index < size; ++index) {
        hashByte(hash, bytes[index]);
    }
    return hash;
}

uint32_t hashSurface(const SDL_Surface* surface)
{
    uint32_t hash = kFnvOffset;
    const auto* pixels = static_cast<const uint8_t*>(surface->pixels);
    for (int y = 0; y < surface->h; ++y) {
        const uint8_t* row = pixels + y * surface->pitch;
        for (int x = 0; x < surface->w; ++x) {
            hashByte(hash, row[x]);
        }
    }
    return hash;
}

uint32_t hashVramVisible(const uint8_t* address)
{
    uint32_t hash = kFnvOffset;
    const auto* words = reinterpret_cast<const volatile u16*>(address);
    for (size_t index = 0; index < kVisibleBytes / sizeof(u16); ++index) {
        const u16 word = words[index];
        hashByte(hash, static_cast<uint8_t>(word & 0xFF));
        hashByte(hash, static_cast<uint8_t>(word >> 8));
    }
    return hash;
}

uint32_t hashSourcePalette()
{
    uint32_t hash = kFnvOffset;
    for (const SDL_Color& color : gSourcePalette) {
        hashByte(hash, color.r);
        hashByte(hash, color.g);
        hashByte(hash, color.b);
        hashByte(hash, color.a);
    }
    return hash;
}

uint32_t hashRgb15Palette(const volatile u16* palette)
{
    uint32_t hash = kFnvOffset;
    for (int index = 0; index < 256; ++index) {
        const u16 value = palette[index];
        hashByte(hash, static_cast<uint8_t>(value & 0xFF));
        hashByte(hash, static_cast<uint8_t>(value >> 8));
    }
    return hash;
}

bool sourcePaletteIsGrayscale()
{
    for (const SDL_Color& color : gSourcePalette) {
        if (color.r != color.g || color.g != color.b) return false;
    }
    return true;
}

void logPaletteEntries()
{
    dsiLog("VIDEO.PALETTE.ENTRY_COUNT=256\n");
    dsiLog("VIDEO.PALETTE.CHANNEL_ORDER=RGB15_R0_G5_B10\n");
    for (int index = 0; index < 16; ++index) {
        const SDL_Color& color = gSourcePalette[index];
        dsiLog("VIDEO.PALETTE[%02d].SOURCE_RGB=%u,%u,%u RESULT_RGB15=0x%04X\n",
            index, color.r, color.g, color.b, gPalette[index]);
    }
    dsiLog("VIDEO.PALETTE.ALL_256_CONVERTED=YES\n");
}

void makeFullDirty()
{
    gDirty = { 0, 0, gSourceWidth, gSourceHeight, true };
    gBottomFullDirty = true;
    gFrameLogPending = true;
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

DirtyRect alignedDirty(DirtyRect dirty)
{
    if (!dirty.valid) return dirty;
    dirty.left = std::max(0, dirty.left & ~3);
    dirty.right = std::min(kScreenWidth, (dirty.right + 3) & ~3);
    dirty.top = std::max(0, dirty.top);
    dirty.bottom = std::min(kScreenHeight, dirty.bottom);
    dirty.valid = dirty.left < dirty.right && dirty.top < dirty.bottom;
    return dirty;
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

void stageFullView(const SDL_Surface* surface, const DsiRectmapView& view,
    uint8_t* stage)
{
    std::memset(stage, 0, kBitmapBytes);
    const DirtyRect fullView = {
        view.destinationX,
        view.destinationY,
        view.destinationX + view.destinationWidth,
        view.destinationY + view.destinationHeight,
        true,
    };
    stageView(surface, view, fullView, stage);
}

void uploadFull(const uint8_t* stage, uint8_t* target)
{
    DC_FlushRange(stage, kVisibleBytes);
    dmaCopyWords(0, stage, target, kVisibleBytes);
}

void uploadDirty(uint8_t* stage, uint8_t* target, DirtyRect dirty)
{
    if (!dirty.valid) return;
    const int width = dirty.right - dirty.left;
    DC_FlushRange(stage + dirty.top * kBitmapStride,
        static_cast<size_t>(dirty.bottom - dirty.top) * kBitmapStride);
    for (int y = dirty.top; y < dirty.bottom; ++y) {
        uint8_t* source = stage + y * kBitmapStride + dirty.left;
        uint8_t* destination = target + y * kBitmapStride + dirty.left;
        dmaCopyWords(0, source, destination, width);
    }
}

void logSurfaceContract(const SDL_Surface* surface)
{
    if (gSurfaceLogged) return;
    const int bitsPerPixel = surface->format != nullptr
        ? surface->format->BitsPerPixel
        : 0;
    const int bytesPerPixel = surface->format != nullptr
        ? surface->format->BytesPerPixel
        : 0;
    const int paletteEntries = surface->format != nullptr
            && surface->format->palette != nullptr
        ? surface->format->palette->ncolors
        : 0;
    dsiLog("VIDEO.SOURCE_WIDTH=%d\n", surface->w);
    dsiLog("VIDEO.SOURCE_HEIGHT=%d\n", surface->h);
    dsiLog("VIDEO.SOURCE_PITCH=%d\n", surface->pitch);
    dsiLog("VIDEO.SOURCE_BPP=%d\n", bitsPerPixel);
    dsiLog("VIDEO.SOURCE_BYTES_PER_PIXEL=%d\n", bytesPerPixel);
    dsiLog("VIDEO.SOURCE_PALETTE_ENTRIES=%d\n", paletteEntries);
    dsiLog("VIDEO.SOURCE_CONTRACT=%s\n",
        surface->w == gSourceWidth && surface->h == gSourceHeight
                && surface->pitch >= surface->w && bitsPerPixel == 8
                && bytesPerPixel == 1 && paletteEntries == 256
            ? "VALID_INDEXED8"
            : "INVALID");
    dsiLog("VIDEO.RENDER_MODE=%s\n", renderModeName());
    gSurfaceLogged = true;
}

} // namespace

bool dsiVideoInit(int sourceWidth, int sourceHeight)
{
    gSourceWidth = sourceWidth;
    gSourceHeight = sourceHeight;
    gMainStage = static_cast<uint8_t*>(memalign(32, kBitmapBytes));
    gSubStage = static_cast<uint8_t*>(memalign(32, kBitmapBytes));
    if (gMainStage == nullptr || gSubStage == nullptr) {
        dsiVideoExit();
        return false;
    }
    std::memset(gMainStage, 0, kBitmapBytes);
    std::memset(gSubStage, 0, kBitmapBytes);

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
    DC_FlushRange(gMainStage, kBitmapBytes);
    DC_FlushRange(gSubStage, kBitmapBytes);
    dmaCopyWords(0, gMainStage, gMainScreen, kBitmapBytes);
    dmaCopyWords(0, gSubStage, gSubScreen, kBitmapBytes);

    for (int index = 0; index < 256; ++index) {
        gSourcePalette[index] = {
            static_cast<Uint8>(index), static_cast<Uint8>(index),
            static_cast<Uint8>(index), 255,
        };
        gPalette[index] = RGB15(index >> 3, index >> 3, index >> 3);
    }
    gPaletteDirty = true;
    gSurfaceLogged = false;
    gPaletteEntriesLogged = false;
    gPaletteIsolationLogged = false;
    gPaletteUpdateSequence = 0;
    dsiRectmapInit(sourceWidth, sourceHeight);
    makeFullDirty();
    bgUpdate();
    dsiLog("VIDEO.SOURCE=%dx%d_INDEXED8\n", sourceWidth, sourceHeight);
    dsiLog("VIDEO.OUTPUT=MAIN_256x192_BG8_SUB_256x192_BG8\n");
    dsiLog("VIDEO.BG_TYPE=BgType_Bmp8\n");
    dsiLog("VIDEO.BG_SIZE=256x256\n");
    dsiLog("VIDEO.DESTINATION_STRIDE=256\n");
    dsiLog("VIDEO.VISIBLE_UPLOAD_BYTES=%u\n", static_cast<unsigned int>(kVisibleBytes));
    dsiLog("VIDEO.DMA_BYTE_ORDER=U16_LOW_INDEX_THEN_HIGH_INDEX\n");
    dsiLog("VIDEO.PALETTE_MEMORY_SEPARATE_FROM_BG_VRAM=YES\n");
    dsiLog("VIDEO.RENDER_MODE=%s\n", renderModeName());
    dsiLog("VIDEO.SELECT_TOGGLE=FULL_2D_REFERENCE<->DIRTY_2D\n");
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
        const SDL_Color& color = colors[first + index];
        gSourcePalette[first + index] = color;
        gPalette[first + index] = RGB15(color.r >> 3, color.g >> 3, color.b >> 3);
    }
    ++gPaletteUpdateSequence;
    // Fallout fades the palette every frame. dsiLog fflushes each line to the
    // SD card, and the two hashes walk 1 KiB, so the diagnostics are capped
    // instead of running on every fade step.
    if (gPaletteUpdateSequence <= kPaletteLogLimit) {
        bool windowMatchesSdl = true;
        for (int index = 0; index < count; ++index) {
            const SDL_Color& expected = colors[first + index];
            const SDL_Color& actual = gSourcePalette[first + index];
            if (expected.r != actual.r || expected.g != actual.g
                || expected.b != actual.b || expected.a != actual.a) {
                windowMatchesSdl = false;
                break;
            }
        }
        dsiLog("VIDEO.PALETTE_UPDATE_SEQUENCE=%u FIRST=%d COUNT=%d\n",
            gPaletteUpdateSequence, first, count);
        dsiLog("VIDEO.PALETTE_SOURCE_HASH=0x%08lX\n",
            static_cast<unsigned long>(hashSourcePalette()));
        dsiLog("VIDEO.PALETTE_RGB15_HASH=0x%08lX\n",
            static_cast<unsigned long>(hashRgb15Palette(gPalette)));
        dsiLog("VIDEO.PALETTE_UPDATE_KIND=%s\n",
            first == 0 && count == 256 ? "FULL_OR_FADE" : "PARTIAL_OR_FADE");
        dsiLog("VIDEO.PALETTE_UPDATED_WINDOW_SYNC=%s\n",
            windowMatchesSdl ? "YES" : "NO");
        if (gPaletteUpdateSequence == kPaletteLogLimit) {
            dsiLog("VIDEO.PALETTE_UPDATE_LOGGING=CAPPED_AFTER_%u\n",
                kPaletteLogLimit);
        }
    }
    if (!gPaletteEntriesLogged && first == 0 && count == 256
        && !sourcePaletteIsGrayscale()) {
        logPaletteEntries();
        gPaletteEntriesLogged = true;
    }
    // Indexed VRAM retains pixel indices. Palette fades only upload palette RAM.
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
    logSurfaceContract(surface);

    const bool fullReference = gStrategy == DsiRenderStrategy::Full2DReference;
    if (!fullReference && !gDirty.valid && !gPaletteDirty && !gBottomFullDirty) return;

    const DsiRectmapView mainView = {
        0, 0, gSourceWidth, gSourceHeight, 0, 0, kScreenWidth, kScreenHeight,
    };
    const DsiRectmapView subView = dsiRectmapBottomView();
    DirtyRect mainUpload = { 0, 0, kScreenWidth, kScreenHeight, true };
    DirtyRect subUpload = { 0, 0, kScreenWidth, kScreenHeight, true };

    if (fullReference) {
        // The reference path deliberately ignores every dirty rectangle.
        stageFullView(surface, mainView, gMainStage);
        stageFullView(surface, subView, gSubStage);
    } else {
        mainUpload = alignedDirty(outputDirty(gDirty, mainView, false));
        DirtyRect subStageDirty = alignedDirty(outputDirty(gDirty, subView,
            gBottomFullDirty));
        subUpload = subStageDirty;
        if (gBottomFullDirty) {
            std::memset(gSubStage, 0, kBitmapBytes);
            subUpload = { 0, 0, kScreenWidth, kScreenHeight, true };
        }
        // Alignment is applied before staging as well as DMA, preventing stale
        // edge indices when a dirty rectangle is widened to DMA word bounds.
        stageView(surface, mainView, mainUpload, gMainStage);
        stageView(surface, subView, subStageDirty, gSubStage);
    }

    uint32_t mainStageHash = 0;
    uint32_t subStageHash = 0;
    if (gFrameLogPending) {
        const uint32_t sourceHash = hashSurface(surface);
        mainStageHash = hashBytes(gMainStage, kVisibleBytes);
        subStageHash = hashBytes(gSubStage, kVisibleBytes);
        dsiLog("VIDEO.RENDER_MODE=%s\n", renderModeName());
        dsiLog("VIDEO.FRAMEBUFFER_SOURCE_HASH=0x%08lX\n",
            static_cast<unsigned long>(sourceHash));
        dsiLog("VIDEO.MAIN_STAGING_HASH=0x%08lX\n",
            static_cast<unsigned long>(mainStageHash));
        dsiLog("VIDEO.BOTTOM_STAGING_HASH=0x%08lX\n",
            static_cast<unsigned long>(subStageHash));
    }

    uint32_t mainBeforePalette = 0;
    uint32_t subBeforePalette = 0;
    if (gPaletteDirty && !gPaletteIsolationLogged) {
        mainBeforePalette = hashVramVisible(gMainScreen);
        subBeforePalette = hashVramVisible(gSubScreen);
    }

    swiWaitForVBlank();
    if (gPaletteDirty) {
        DC_FlushRange(gPalette, sizeof(gPalette));
        dmaCopyWords(0, gPalette, BG_PALETTE, sizeof(gPalette));
        dmaCopyWords(0, gPalette, BG_PALETTE_SUB, sizeof(gPalette));
        // Same reason as the update log: a fade makes this per-frame work.
        if (gPaletteUpdateSequence <= kPaletteLogLimit) {
            const uint32_t expectedPaletteHash = hashRgb15Palette(gPalette);
            const uint32_t mainPaletteHash = hashRgb15Palette(BG_PALETTE);
            const uint32_t subPaletteHash = hashRgb15Palette(BG_PALETTE_SUB);
            dsiLog("VIDEO.PALETTE_MAIN_READBACK_HASH=0x%08lX MATCH=%s\n",
                static_cast<unsigned long>(mainPaletteHash),
                mainPaletteHash == expectedPaletteHash ? "YES" : "NO");
            dsiLog("VIDEO.PALETTE_SUB_READBACK_HASH=0x%08lX MATCH=%s\n",
                static_cast<unsigned long>(subPaletteHash),
                subPaletteHash == expectedPaletteHash ? "YES" : "NO");
        }
        if (!gPaletteIsolationLogged) {
            const uint32_t mainAfterPalette = hashVramVisible(gMainScreen);
            const uint32_t subAfterPalette = hashVramVisible(gSubScreen);
            dsiLog("VIDEO.PALETTE_UPLOAD_MAIN_BG_UNCHANGED=%s\n",
                mainAfterPalette == mainBeforePalette ? "YES" : "NO");
            dsiLog("VIDEO.PALETTE_UPLOAD_SUB_BG_UNCHANGED=%s\n",
                subAfterPalette == subBeforePalette ? "YES" : "NO");
            gPaletteIsolationLogged = true;
        }
    }

    if (fullReference) {
        uploadFull(gMainStage, gMainScreen);
        uploadFull(gSubStage, gSubScreen);
    } else {
        uploadDirty(gMainStage, gMainScreen, mainUpload);
        uploadDirty(gSubStage, gSubScreen, subUpload);
    }
    bgUpdate();

    if (gFrameLogPending) {
        const uint32_t mainVramHash = hashVramVisible(gMainScreen);
        const uint32_t subVramHash = hashVramVisible(gSubScreen);
        dsiLog("VIDEO.MAIN_VRAM_READBACK_HASH=0x%08lX MATCH_STAGING=%s\n",
            static_cast<unsigned long>(mainVramHash),
            mainVramHash == mainStageHash ? "YES" : "NO");
        dsiLog("VIDEO.BOTTOM_VRAM_READBACK_HASH=0x%08lX MATCH_STAGING=%s\n",
            static_cast<unsigned long>(subVramHash),
            subVramHash == subStageHash ? "YES" : "NO");
        gFrameLogPending = false;
    }

    gDirty.valid = false;
    gPaletteDirty = false;
    gBottomFullDirty = false;
}

void dsiVideoSetStrategy(DsiRenderStrategy strategy)
{
    gStrategy = strategy;
    makeFullDirty();
    dsiLog("VIDEO.RENDER_MODE=%s\n", renderModeName());
}

void dsiVideoToggleStrategy()
{
    dsiVideoSetStrategy(gStrategy == DsiRenderStrategy::Full2DReference
            ? DsiRenderStrategy::Dirty2D
            : DsiRenderStrategy::Full2DReference);
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
    gFrameLogPending = true;
}

} // namespace fallout
