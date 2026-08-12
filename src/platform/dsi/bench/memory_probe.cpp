#include "memory_probe.h"

#include <malloc.h>
#include <cstdlib>

namespace dsi_bench {

static uint32_t gCurrentBytes;
static uint32_t gPeakBytes;
static uint32_t gCurrentBlocks;
static uint32_t gPeakBlocks;

void* trackedAlloc(size_t bytes)
{
    void* pointer = std::malloc(bytes);
    if (pointer != nullptr) {
        gCurrentBytes += static_cast<uint32_t>(bytes);
        ++gCurrentBlocks;
        if (gCurrentBytes > gPeakBytes) {
            gPeakBytes = gCurrentBytes;
        }
        if (gCurrentBlocks > gPeakBlocks) {
            gPeakBlocks = gCurrentBlocks;
        }
    }
    return pointer;
}

void trackedFree(void* pointer, size_t bytes)
{
    if (pointer == nullptr) {
        return;
    }
    std::free(pointer);
    gCurrentBytes -= static_cast<uint32_t>(bytes);
    --gCurrentBlocks;
}

uint32_t trackedCurrentBytes()
{
    return gCurrentBytes;
}

uint32_t trackedPeakBytes()
{
    return gPeakBytes;
}

uint32_t trackedCurrentBlocks()
{
    return gCurrentBlocks;
}

uint32_t trackedPeakBlocks()
{
    return gPeakBlocks;
}

static uint32_t platformFreeBytes()
{
    const struct mallinfo info = mallinfo();
    const uintptr_t heapEnd = reinterpret_cast<uintptr_t>(getHeapEnd());
    const uintptr_t heapLimit = reinterpret_cast<uintptr_t>(getHeapLimit());
    const uint32_t tail = heapLimit > heapEnd
        ? static_cast<uint32_t>(heapLimit - heapEnd)
        : 0;
    return static_cast<uint32_t>(info.fordblks) + tail;
}

static uint32_t largestAllocatableBlock()
{
    constexpr uint32_t granularity = 4096;
    uint32_t low = 0;
    uint32_t high = platformFreeBytes() / granularity;

    while (low < high) {
        const uint32_t middle = low + (high - low + 1) / 2;
        const size_t bytes = static_cast<size_t>(middle) * granularity;
        void* allocation = std::malloc(bytes);
        if (allocation != nullptr) {
            std::free(allocation);
            low = middle;
        } else {
            high = middle - 1;
        }
    }
    return low * granularity;
}

MemorySnapshot captureMemorySnapshot(bool probeLargestBlock)
{
    const uintptr_t heapStart = reinterpret_cast<uintptr_t>(getHeapStart());
    const uintptr_t heapEndBeforeProbe = reinterpret_cast<uintptr_t>(getHeapEnd());
    const uintptr_t heapLimit = reinterpret_cast<uintptr_t>(getHeapLimit());
    const uint32_t largest = probeLargestBlock ? largestAllocatableBlock() : 0;
    const uint32_t freeBytes = platformFreeBytes();
    return {
        trackedCurrentBytes(),
        trackedPeakBytes(),
        trackedCurrentBlocks(),
        trackedPeakBlocks(),
        static_cast<uint32_t>(heapStart),
        static_cast<uint32_t>(heapEndBeforeProbe),
        static_cast<uint32_t>(heapLimit),
        freeBytes,
        largest,
        probeLargestBlock,
    };
}

void logMemorySnapshot(Logger& log, const char* checkpoint,
    const char* category, const MemorySnapshot& snapshot)
{
    char key[112];
#define LOG_MEMORY_VALUE(name, value) \
    do { \
        std::snprintf(key, sizeof(key), "%s.%s", checkpoint, name); \
        log.line("MEMORY", key, static_cast<uint32_t>(value)); \
    } while (0)

    std::snprintf(key, sizeof(key), "%s.CATEGORY", checkpoint);
    log.line("MEMORY", key, category);
    LOG_MEMORY_VALUE("CURRENT_BYTES", snapshot.currentBytes);
    LOG_MEMORY_VALUE("PEAK_BYTES", snapshot.peakBytes);
    LOG_MEMORY_VALUE("CURRENT_BLOCKS", snapshot.currentBlocks);
    LOG_MEMORY_VALUE("PEAK_BLOCKS", snapshot.peakBlocks);
    LOG_MEMORY_VALUE("HEAP_START", snapshot.heapStart);
    LOG_MEMORY_VALUE("HEAP_END_BEFORE_PROBE", snapshot.heapEndBeforeProbe);
    LOG_MEMORY_VALUE("HEAP_LIMIT", snapshot.heapLimit);
    LOG_MEMORY_VALUE("PLATFORM_FREE_BYTES", snapshot.platformFreeBytes);
    LOG_MEMORY_VALUE("LARGEST_ALLOCATABLE_BLOCK", snapshot.largestAllocatableBlock);
    LOG_MEMORY_VALUE("LARGEST_BLOCK_PROBED", snapshot.largestBlockProbed ? 1 : 0);
#undef LOG_MEMORY_VALUE
    log.flush();
}

void logMemoryCheckpoint(Logger& log, const char* checkpoint,
    const char* category, bool probeLargestBlock)
{
    logMemorySnapshot(log, checkpoint, category,
        captureMemorySnapshot(probeLargestBlock));
}

void logUnavailableRuntimeCheckpoints(Logger& log)
{
    static const char* const checkpoints[] = {
        "03_AFTER_ENGINE_INITIALIZATION",
        "05_BEFORE_REPRESENTATIVE_MAP_LOAD",
        "06_AFTER_MAP_OBJECTS_SCRIPTS_MESSAGES",
        "07_AFTER_ART_PRELOAD_SETTLES",
        "08_AFTER_60S_EXPLORATION_AUDIO_OFF",
        "09_AFTER_COMBAT_INVENTORY_DIALOG_AUDIO_OFF",
        "10_REPEAT_STATE_AUDIO_ON",
        "11_SAVE_LOAD_PEAK_WINDOWS",
        "12_AFTER_MAP_UNLOAD",
    };
    for (const char* checkpoint : checkpoints) {
        char key[112];
        std::snprintf(key, sizeof(key), "%s.RUNTIME_STATUS", checkpoint);
        log.line("MEMORY", key, "NOT_RUN_NO_FALLOUT_RUNTIME");
    }
    log.line("MEMORY", "AUDIO_OFF_PEAK_BYTES", "UNKNOWN_NO_FALLOUT_RUNTIME");
    log.line("MEMORY", "AUDIO_ON_PEAK_BYTES", "UNKNOWN_NO_FALLOUT_RUNTIME");
    log.flush();
}

} // namespace dsi_bench
