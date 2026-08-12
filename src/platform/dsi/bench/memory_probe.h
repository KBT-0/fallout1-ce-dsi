#pragma once

#include "bench.h"

#include <cstddef>
#include <cstdint>

namespace dsi_bench {

void* trackedAlloc(size_t bytes);
void trackedFree(void* pointer, size_t bytes);
uint32_t trackedCurrentBytes();
uint32_t trackedPeakBytes();
uint32_t trackedCurrentBlocks();
uint32_t trackedPeakBlocks();

struct MemorySnapshot {
    uint32_t currentBytes;
    uint32_t peakBytes;
    uint32_t currentBlocks;
    uint32_t peakBlocks;
    uint32_t heapStart;
    uint32_t heapEndBeforeProbe;
    uint32_t heapLimit;
    uint32_t platformFreeBytes;
    uint32_t largestAllocatableBlock;
    bool largestBlockProbed;
};

MemorySnapshot captureMemorySnapshot(bool probeLargestBlock = true);
void logMemorySnapshot(Logger& log, const char* checkpoint,
    const char* category, const MemorySnapshot& snapshot);
void logMemoryCheckpoint(Logger& log, const char* checkpoint,
    const char* category, bool probeLargestBlock = true);
void logUnavailableRuntimeCheckpoints(Logger& log);

} // namespace dsi_bench
