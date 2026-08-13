#include "dsi_rectmap.h"

#include "dsi_runtime.h"
#include "dsi_video.h"

#include <algorithm>

namespace fallout {
namespace {

int gSourceWidth = 640;
int gSourceHeight = 480;
DsiRectmapMode gMode = DsiRectmapMode::Full;

DsiRectmapView fitView(int x, int y, int width, int height)
{
    const int scaledWidth = std::min(256, width * 192 / height);
    const int scaledHeight = std::min(192, height * 256 / width);
    return {
        x,
        y,
        width,
        height,
        (256 - scaledWidth) / 2,
        (192 - scaledHeight) / 2,
        scaledWidth,
        scaledHeight,
    };
}

} // namespace

void dsiRectmapInit(int sourceWidth, int sourceHeight)
{
    gSourceWidth = sourceWidth;
    gSourceHeight = sourceHeight;
    gMode = DsiRectmapMode::Full;
}

void dsiRectmapSetMode(DsiRectmapMode mode)
{
    if (gMode == mode) return;
    gMode = mode;
    dsiLog("RECTMAP.MODE=%d\n", static_cast<int>(mode));
    dsiVideoRectmapChanged();
}

DsiRectmapMode dsiRectmapMode()
{
    return gMode;
}

DsiRectmapView dsiRectmapBottomView()
{
    switch (gMode) {
    case DsiRectmapMode::MainMenu:
        // DISPLAY_MAIN from the 3DS port, with its 320x240 destination scaled
        // exactly to the DSi's 256x192 touch screen.
        return { 400, 25, 215, 230, 34, 0, 187, 192 };
    case DsiRectmapMode::Field:
        return fitView(0, 0, 400, 240);
    case DsiRectmapMode::Gui:
        return fitView(0, 359, 640, 121);
    case DsiRectmapMode::Dialog:
        return fitView(145, 320, 345, 160);
    case DsiRectmapMode::Inventory:
        return fitView(95, 10, 470, 357);
    case DsiRectmapMode::Character:
        return fitView(10, 30, 625, 420);
    case DsiRectmapMode::LoadSave:
        return fitView(20, 20, 600, 440);
    case DsiRectmapMode::WorldMap:
    case DsiRectmapMode::PipBoy:
    case DsiRectmapMode::Full:
    default:
        return fitView(0, 0, gSourceWidth, gSourceHeight);
    }
}

void dsiRectmapMapTouch(int screenX, int screenY, int* sourceX, int* sourceY)
{
    const DsiRectmapView view = dsiRectmapBottomView();
    const int x = std::clamp(screenX, view.destinationX,
        view.destinationX + view.destinationWidth - 1);
    const int y = std::clamp(screenY, view.destinationY,
        view.destinationY + view.destinationHeight - 1);
    if (sourceX != nullptr) {
        *sourceX = view.sourceX
            + (x - view.destinationX) * view.sourceWidth / view.destinationWidth;
    }
    if (sourceY != nullptr) {
        *sourceY = view.sourceY
            + (y - view.destinationY) * view.sourceHeight / view.destinationHeight;
    }
}

} // namespace fallout
