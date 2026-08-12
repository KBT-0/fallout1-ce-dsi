#pragma once

#include <cstdint>

namespace dsi_bench {

struct Rect {
    int x;
    int y;
    int width;
    int height;
};

struct Point {
    int x;
    int y;
};

struct RectMap {
    Rect source;
    Rect destination;
};

bool mapPoint(const RectMap& map, Point destinationPoint, Point* sourcePoint);
bool runRectMapSelfTest(uint32_t* passed, uint32_t* failed);

} // namespace dsi_bench
