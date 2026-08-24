// Deterministic host test for the production DSi video backend.
//
// The reference expectation is computed independently of dsi_video.cc so a
// pitch, stride, or scaling mistake in the backend cannot cancel itself out.

#include "platform/dsi/runtime/dsi_rectmap.h"
#include "platform/dsi/runtime/dsi_video.h"

#include <nds.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr int kSourceWidth = 640;
constexpr int kSourceHeight = 480;
// Deliberately padded: nothing in the backend may assume pitch == width.
constexpr int kSourcePitch = 704;
constexpr int kScreenWidth = 256;
constexpr int kScreenHeight = 192;

SDL_Palette gPalette;
SDL_PixelFormat gFormat;
SDL_Surface gSurface;
std::vector<Uint8> gPixels;
std::vector<SDL_Color> gColors;

void makeSurface()
{
    gPixels.assign(static_cast<size_t>(kSourcePitch) * kSourceHeight, 0xEE);
    gColors.assign(256, SDL_Color {});
    gPalette.ncolors = 256;
    gPalette.colors = gColors.data();
    gFormat.format = 0;
    gFormat.palette = &gPalette;
    gFormat.BitsPerPixel = 8;
    gFormat.BytesPerPixel = 1;
    gSurface.flags = 0;
    gSurface.format = &gFormat;
    gSurface.w = kSourceWidth;
    gSurface.h = kSourceHeight;
    gSurface.pitch = kSourcePitch;
    gSurface.pixels = gPixels.data();
    gSurface.ownsPalette = 0;
}

Uint8& pixel(int x, int y) { return gPixels[static_cast<size_t>(y) * kSourcePitch + x]; }

void fillPattern(unsigned int seed)
{
    for (int y = 0; y < kSourceHeight; ++y) {
        for (int x = 0; x < kSourceWidth; ++x) {
            pixel(x, y) = static_cast<Uint8>((x * 7 + y * 13 + seed * 31) & 0xFF);
        }
    }
}

// Independent nearest-neighbour reference for one rectmap view.
std::vector<u8> reference(const fallout::DsiRectmapView& view)
{
    std::vector<u8> out(static_cast<size_t>(kScreenWidth) * kScreenHeight, 0);
    for (int y = view.destinationY; y < view.destinationY + view.destinationHeight; ++y) {
        for (int x = view.destinationX; x < view.destinationX + view.destinationWidth; ++x) {
            const int sy = view.sourceY
                + (y - view.destinationY) * view.sourceHeight / view.destinationHeight;
            const int sx = view.sourceX
                + (x - view.destinationX) * view.sourceWidth / view.destinationWidth;
            assert(sx >= 0 && sx < kSourceWidth && sy >= 0 && sy < kSourceHeight);
            out[static_cast<size_t>(y) * kScreenWidth + x] = pixel(sx, sy);
        }
    }
    return out;
}

int gFailures;

void expectVram(const char* label, const u8* vram, const std::vector<u8>& want)
{
    for (int y = 0; y < kScreenHeight; ++y) {
        for (int x = 0; x < kScreenWidth; ++x) {
            const size_t index = static_cast<size_t>(y) * kScreenWidth + x;
            if (vram[index] != want[index]) {
                std::printf("FAIL %s: (%d,%d) got %u want %u\n", label, x, y,
                    vram[index], want[index]);
                ++gFailures;
                return;
            }
        }
    }
    std::printf("ok   %s\n", label);
}

fallout::DsiRectmapView mainView()
{
    return { 0, 0, kSourceWidth, kSourceHeight, 0, 0, kScreenWidth, kScreenHeight };
}

void checkBothScreens(const char* label)
{
    expectVram(label, gHostMainVram, reference(mainView()));
    std::string bottom = std::string(label) + " (bottom)";
    expectVram(bottom.c_str(), gHostSubVram, reference(fallout::dsiRectmapBottomView()));
}

} // namespace

int main()
{
    makeSurface();
    fillPattern(0);

    const bool ok = fallout::dsiVideoInit(kSourceWidth, kSourceHeight);
    if (!ok) {
        std::printf("FAIL dsiVideoInit returned false\n");
        return 1;
    }

    // 1. Full reference path, Full rectmap.
    fallout::dsiVideoSetStrategy(fallout::DsiRenderStrategy::Full2DReference);
    fallout::dsiVideoPresent(&gSurface);
    checkBothScreens("full reference, Full rectmap");

    // 2. Full reference path, MainMenu rectmap (offset/letterboxed bottom view).
    fallout::dsiRectmapSetMode(fallout::DsiRectmapMode::MainMenu);
    fallout::dsiVideoPresent(&gSurface);
    checkBothScreens("full reference, MainMenu rectmap");

    // 3. Palette conversion: 6-bit Fallout entries arrive pre-scaled by <<2.
    {
        std::vector<SDL_Color> colors(256);
        for (int i = 0; i < 256; ++i) {
            colors[i].r = static_cast<Uint8>((i & 63) << 2);
            colors[i].g = static_cast<Uint8>(((i + 21) & 63) << 2);
            colors[i].b = static_cast<Uint8>(((i + 42) & 63) << 2);
            colors[i].a = 255;
        }
        fallout::dsiVideoSetPalette(colors.data(), 0, 256);
        fallout::dsiVideoPresent(&gSurface);
        int bad = 0;
        for (int i = 0; i < 256; ++i) {
            const u16 want = RGB15(colors[i].r >> 3, colors[i].g >> 3, colors[i].b >> 3);
            if (gHostMainPalette[i] != want || gHostSubPalette[i] != want) ++bad;
        }
        if (bad != 0) {
            std::printf("FAIL palette: %d entries wrong\n", bad);
            ++gFailures;
        } else {
            std::printf("ok   palette RGB15 conversion (main and sub)\n");
        }
    }

    // 4. Dirty path must land on the same image as the reference path for the
    //    same final framebuffer, across a sequence of marked updates.
    for (int mode = 0; mode < 2; ++mode) {
        fallout::dsiRectmapSetMode(mode == 0 ? fallout::DsiRectmapMode::Full
                                             : fallout::DsiRectmapMode::MainMenu);
        fallout::dsiVideoSetStrategy(fallout::DsiRenderStrategy::Full2DReference);
        fillPattern(1);
        fallout::dsiVideoPresent(&gSurface);

        fallout::dsiVideoSetStrategy(fallout::DsiRenderStrategy::Dirty2D);
        fallout::dsiVideoPresent(&gSurface); // strategy switch forces a full redraw

        const struct { int x, y, w, h; } updates[] = {
            { 0, 0, 3, 3 },
            { 1, 1, 1, 1 },
            { 637, 477, 3, 3 },
            { 100, 200, 37, 41 },
            { 399, 24, 217, 232 },
            { 5, 5, 630, 470 },
            { 0, 0, 640, 480 },
        };
        for (unsigned int i = 0; i < sizeof(updates) / sizeof(updates[0]); ++i) {
            const auto& u = updates[i];
            // Touch only what is reported dirty; that is the contract the
            // engine's GNW95_ShowRect actually honours.
            for (int y = u.y; y < u.y + u.h; ++y) {
                for (int x = u.x; x < u.x + u.w; ++x) {
                    pixel(x, y) = static_cast<Uint8>((x * 3 + y * 5 + (i + 2) * 97) & 0xFF);
                }
            }
            fallout::dsiVideoMarkDirty(u.x, u.y, u.w, u.h);
            fallout::dsiVideoPresent(&gSurface);
        }
        char label[96];
        std::snprintf(label, sizeof(label), "dirty path matches reference (rectmap %d)", mode);
        checkBothScreens(label);
    }

    if (gFailures != 0) {
        std::printf("DSi video host test: %d FAILURE(S)\n", gFailures);
        return 1;
    }
    std::printf("DSi video host test: PASS\n");
    return 0;
}
