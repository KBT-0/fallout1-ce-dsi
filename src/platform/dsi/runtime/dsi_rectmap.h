#pragma once

namespace fallout {

enum class DsiRectmapMode {
    Full,
    MainMenu,
    Field,
    Gui,
    Dialog,
    Inventory,
    Character,
    LoadSave,
    WorldMap,
    PipBoy,
};

struct DsiRectmapView {
    int sourceX;
    int sourceY;
    int sourceWidth;
    int sourceHeight;
    int destinationX;
    int destinationY;
    int destinationWidth;
    int destinationHeight;
};

void dsiRectmapInit(int sourceWidth, int sourceHeight);
void dsiRectmapSetMode(DsiRectmapMode mode);
DsiRectmapMode dsiRectmapMode();
DsiRectmapView dsiRectmapBottomView();
void dsiRectmapMapTouch(int screenX, int screenY, int* sourceX, int* sourceY);

} // namespace fallout
