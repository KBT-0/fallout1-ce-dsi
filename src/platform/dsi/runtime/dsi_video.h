#pragma once

#include <SDL.h>

namespace fallout {

enum class DsiRenderStrategy {
    Full2DReference,
    Dirty2D,
};

bool dsiVideoInit(int sourceWidth, int sourceHeight);
void dsiVideoExit();
void dsiVideoSetPalette(const SDL_Color* colors, int first, int count);
void dsiVideoMarkDirty(int x, int y, int width, int height);
void dsiVideoPresent(const SDL_Surface* indexedSurface);
void dsiVideoSetStrategy(DsiRenderStrategy strategy);
void dsiVideoToggleStrategy();
DsiRenderStrategy dsiVideoStrategy();
void dsiVideoMapTouch(int screenX, int screenY, int* sourceX, int* sourceY);
void dsiVideoRectmapChanged();

} // namespace fallout
