#pragma once

#include "bench.h"

#include <cstddef>
#include <cstdint>

namespace dsi_bench {

struct RenderResources {
    uint8_t* source;
    uint8_t* mainStage;
    uint8_t* subStage;
    uint8_t* textureStage;
};

bool allocateRenderResources(RenderResources* resources);
void freeRenderResources(RenderResources* resources);
bool runRenderBenchmark(Logger& log, RenderResources& resources);

} // namespace dsi_bench
