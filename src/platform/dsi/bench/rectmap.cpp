#include "rectmap.h"

namespace dsi_bench {

bool mapPoint(const RectMap& map, Point destinationPoint, Point* sourcePoint)
{
    if (sourcePoint == nullptr || map.source.width <= 0 || map.source.height <= 0
        || map.destination.width <= 0 || map.destination.height <= 0) {
        return false;
    }
    if (destinationPoint.x < map.destination.x
        || destinationPoint.y < map.destination.y
        || destinationPoint.x >= map.destination.x + map.destination.width
        || destinationPoint.y >= map.destination.y + map.destination.height) {
        return false;
    }

    const int relativeX = destinationPoint.x - map.destination.x;
    const int relativeY = destinationPoint.y - map.destination.y;
    sourcePoint->x = map.source.x
        + (relativeX * map.source.width) / map.destination.width;
    sourcePoint->y = map.source.y
        + (relativeY * map.source.height) / map.destination.height;
    return true;
}

static bool expectMap(const RectMap& map, Point input, bool expectedInside,
    Point expectedOutput)
{
    Point actual = { -1, -1 };
    const bool inside = mapPoint(map, input, &actual);
    return inside == expectedInside
        && (!inside || (actual.x == expectedOutput.x && actual.y == expectedOutput.y));
}

bool runRectMapSelfTest(uint32_t* passed, uint32_t* failed)
{
    uint32_t localPassed = 0;
    uint32_t localFailed = 0;
    const RectMap full = { { 0, 0, 640, 480 }, { 0, 0, 256, 192 } };
    const RectMap inset = { { 100, 50, 320, 200 }, { 16, 8, 160, 100 } };

    const bool cases[] = {
        expectMap(full, { 0, 0 }, true, { 0, 0 }),
        expectMap(full, { 128, 96 }, true, { 320, 240 }),
        expectMap(full, { 255, 191 }, true, { 637, 477 }),
        expectMap(full, { -1, 0 }, false, { 0, 0 }),
        expectMap(full, { 256, 191 }, false, { 0, 0 }),
        expectMap(inset, { 16, 8 }, true, { 100, 50 }),
        expectMap(inset, { 96, 58 }, true, { 260, 150 }),
        expectMap(inset, { 175, 107 }, true, { 418, 248 }),
        expectMap(inset, { 15, 8 }, false, { 0, 0 }),
    };

    for (bool result : cases) {
        if (result) {
            ++localPassed;
        } else {
            ++localFailed;
        }
    }
    if (passed != nullptr) {
        *passed = localPassed;
    }
    if (failed != nullptr) {
        *failed = localFailed;
    }
    return localFailed == 0;
}

} // namespace dsi_bench
