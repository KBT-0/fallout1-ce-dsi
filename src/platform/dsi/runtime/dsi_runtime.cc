#include "dsi_runtime.h"

#include <fat.h>
#include <nds.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <sys/stat.h>
#include <unistd.h>

#include "plib/gnw/memory.h"

namespace fallout {
namespace {

constexpr const char* kRoot = "sd:/fallout1";
constexpr const char* kOriginalRoot = "sd:/fallout1/original";
constexpr const char* kLogPath = "sd:/fallout1/fallout-dsi.log";

FILE* gLog;
char gLastStage[96] = "PROCESS_ENTRY";

void showConsole(const char* heading, const char* message)
{
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_LCD);
    vramSetBankB(VRAM_B_LCD);
    consoleDemoInit();
    consoleClear();
    iprintf("%s\n\n", heading);
    iprintf("Stage:\n%s\n\n", gLastStage);
    if (message != nullptr) iprintf("%s\n", message);
    iprintf("\nLog:\n/fallout1/fallout-dsi.log\n");
    iprintf("\nPress START to exit.\n");
}

uint32_t platformFreeBytes()
{
    const struct mallinfo info = mallinfo();
    const uintptr_t heapEnd = reinterpret_cast<uintptr_t>(getHeapEnd());
    const uintptr_t heapLimit = reinterpret_cast<uintptr_t>(getHeapLimit());
    const uint32_t tail = heapLimit > heapEnd
        ? static_cast<uint32_t>(heapLimit - heapEnd)
        : 0;
    return static_cast<uint32_t>(info.fordblks) + tail;
}

uint32_t largestAllocatableBlock()
{
    constexpr uint32_t kGranularity = 4096;
    uint32_t low = 0;
    uint32_t high = platformFreeBytes() / kGranularity;
    while (low < high) {
        const uint32_t middle = low + (high - low + 1) / 2;
        void* allocation = std::malloc(static_cast<size_t>(middle) * kGranularity);
        if (allocation != nullptr) {
            std::free(allocation);
            low = middle;
        } else {
            high = middle - 1;
        }
    }
    return low * kGranularity;
}

struct PlatformMemorySnapshot {
    uintptr_t heapStart;
    uintptr_t heapEnd;
    uintptr_t heapLimit;
    uint32_t freeBytes;
    uint32_t largestBlock;
    bool largestBlockProbed;
};

PlatformMemorySnapshot capturePlatformMemory(bool probeLargestBlock)
{
    const uintptr_t heapStart = reinterpret_cast<uintptr_t>(getHeapStart());
    const uintptr_t heapEnd = reinterpret_cast<uintptr_t>(getHeapEnd());
    const uintptr_t heapLimit = reinterpret_cast<uintptr_t>(getHeapLimit());
    const uint32_t largest = probeLargestBlock ? largestAllocatableBlock() : 0;
    return { heapStart, heapEnd, heapLimit, platformFreeBytes(), largest,
        probeLargestBlock };
}

void logMemorySnapshot(const char* checkpoint,
    const PlatformMemorySnapshot& snapshot)
{
    MemoryStats engine = {};
    mem_get_stats(&engine);
    dsiLog("MEMORY.%s.HEAP_START=%lu\n", checkpoint,
        static_cast<unsigned long>(snapshot.heapStart));
    dsiLog("MEMORY.%s.HEAP_END=%lu\n", checkpoint,
        static_cast<unsigned long>(snapshot.heapEnd));
    dsiLog("MEMORY.%s.HEAP_LIMIT=%lu\n", checkpoint,
        static_cast<unsigned long>(snapshot.heapLimit));
    dsiLog("MEMORY.%s.PLATFORM_FREE_BYTES=%lu\n", checkpoint,
        static_cast<unsigned long>(snapshot.freeBytes));
    dsiLog("MEMORY.%s.LARGEST_ALLOCATABLE_BLOCK=%lu\n", checkpoint,
        static_cast<unsigned long>(snapshot.largestBlock));
    dsiLog("MEMORY.%s.LARGEST_BLOCK_PROBED=%s\n", checkpoint,
        snapshot.largestBlockProbed ? "YES" : "NO");
    dsiLog("MEMORY.%s.GNW_CURRENT_BYTES=%lu\n", checkpoint,
        static_cast<unsigned long>(engine.currentBytes));
    dsiLog("MEMORY.%s.GNW_PEAK_BYTES=%lu\n", checkpoint,
        static_cast<unsigned long>(engine.peakBytes));
    dsiLog("MEMORY.%s.GNW_CURRENT_BLOCKS=%d\n", checkpoint,
        engine.currentBlocks);
    dsiLog("MEMORY.%s.GNW_PEAK_BLOCKS=%d\n", checkpoint,
        engine.peakBlocks);
}

} // namespace

bool dsiRuntimeInit()
{
    const PlatformMemorySnapshot processEntry = capturePlatformMemory(true);
    std::strcpy(gLastStage, "BOOT");
    consoleDemoInit();
    consoleClear();
    iprintf("Fallout 1 CE DSi\nStarting...\n");

    if (!fatInitDefault()) {
        std::strcpy(gLastStage, "FAT_INIT_FAILED");
        showConsole("Startup failed", "Cannot initialize SD/FAT.");
        return false;
    }
    mkdir(kRoot, 0777);
    gLog = std::fopen(kLogPath, "w");
    if (gLog == nullptr) {
        std::strcpy(gLastStage, "LOG_OPEN_FAILED");
        showConsole("Startup failed", "Cannot create fallout-dsi.log.");
        return false;
    }
    std::setvbuf(gLog, nullptr, _IOLBF, 0);
    dsiLog("RUN.FORMAT=FALLOUT1_DSI_RUNTIME_V1\n");
    dsiLog("RUN.DATASET=ORIGINAL\n");
    dsiLog("RUN.DATA_ROOT=%s\n", kOriginalRoot);
    dsiLog("RUN.DSI_MODE=%s\n", isDSiMode() ? "YES" : "NO");
    dsiStartupStage("BOOT");
    logMemorySnapshot("01_PROCESS_ENTRY", processEntry);
    if (!isDSiMode()) {
        dsiStartupStage("NOT_DSI_MODE");
        dsiFatal("This build requires DSi mode.");
        return false;
    }
    dsiStartupStage("DSI_MODE_OK");
    setCpuClock(true);
    dsiStartupStage("FILESYSTEM_OK");
    dsiLogMemory("02_AFTER_PLATFORM_FILESYSTEM_INITIALIZATION", true);
    if (chdir(kOriginalRoot) != 0) {
        dsiStartupStage("ORIGINAL_CHDIR_FAILED");
        dsiFatal("Missing sd:/fallout1/original/.");
        return false;
    }
    dsiStartupStage("DATA_ROOT_SELECTED");
    return true;
}

void dsiRuntimeShutdown()
{
    dsiLog("RUN.END_STAGE=%s\n", gLastStage);
    if (gLog != nullptr) {
        std::fclose(gLog);
        gLog = nullptr;
    }
}

void dsiStartupStage(const char* stage)
{
    if (stage == nullptr) return;
    std::snprintf(gLastStage, sizeof(gLastStage), "%s", stage);
    dsiLog("STARTUP.STAGE=%s\n", gLastStage);
}

const char* dsiLastStartupStage()
{
    return gLastStage;
}

void dsiLogV(const char* format, va_list args)
{
    if (gLog != nullptr) {
        std::vfprintf(gLog, format, args);
        std::fflush(gLog);
    }
}

void dsiLog(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    dsiLogV(format, args);
    va_end(args);
}

void dsiFatal(const char* message)
{
    dsiLog("FATAL.STAGE=%s\n", gLastStage);
    dsiLog("FATAL.MESSAGE=%s\n", message != nullptr ? message : "UNKNOWN");
    if (gLog != nullptr) std::fflush(gLog);
    showConsole("Fallout DSi failure", message);
    while (pmMainLoop()) {
        scanKeys();
        if ((keysDown() & KEY_START) != 0) break;
        swiWaitForVBlank();
    }
}

void dsiLogMemory(const char* checkpoint, bool probeLargestBlock)
{
    if (checkpoint == nullptr) return;
    logMemorySnapshot(checkpoint, capturePlatformMemory(probeLargestBlock));
}

} // namespace fallout
