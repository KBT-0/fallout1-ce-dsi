#include "bench.h"
#include "input_selftest.h"
#include "memory_probe.h"
#include "rectmap.h"
#include "render_bench.h"
#include "sd_bench.h"

#include <fat.h>
#include <nds.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#ifndef DSI_BENCH_GIT_REVISION
#define DSI_BENCH_GIT_REVISION "unknown"
#endif

namespace {

struct RunSelection {
    const char* environment;
    const char* launcher;
    bool appendLog;
};

uint32_t waitForAnyKey(uint32_t mask)
{
    do {
        swiWaitForVBlank();
        scanKeys();
    } while (keysHeld() != 0);
    while (pmMainLoop()) {
        swiWaitForVBlank();
        scanKeys();
        const uint32_t pressed = keysDown() & mask;
        if (pressed != 0) {
            return pressed;
        }
    }
    return 0;
}

RunSelection selectRun()
{
    consoleClear();
    iprintf("Fallout 1 DSi Phase 0\n");
    iprintf("Unified benchmark\n\n");
    iprintf("Run environment:\n");
    iprintf("A = real DSi\nX = melonDS\nY = other\n");
    const uint32_t environmentKey = waitForAnyKey(KEY_A | KEY_X | KEY_Y);
    const char* environment = environmentKey & KEY_A ? "REAL_DSI"
        : environmentKey & KEY_X ? "MELONDS"
                                 : "OTHER";

    consoleClear();
    iprintf("Launcher:\n\n");
    iprintf("L = direct Unlaunch\n");
    iprintf("    (new bench.log)\n");
    iprintf("R = TWiLight Menu++\n");
    iprintf("    (append bench.log)\n");
    iprintf("Y = other (new log)\n");
    const uint32_t launcherKey = waitForAnyKey(KEY_L | KEY_R | KEY_Y);
    if (launcherKey & KEY_L) {
        return { environment, "DIRECT_UNLAUNCH", false };
    }
    if (launcherKey & KEY_R) {
        return { environment, "TWILIGHT_MENU_PLUS_PLUS", true };
    }
    return { environment, "OTHER", false };
}

void restoreConsole()
{
    while (REG_DISPCAPCNT & DCAP_ENABLE) {
    }
    lcdMainOnTop();
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_LCD);
    vramSetBankB(VRAM_B_LCD);
    vramSetBankD(VRAM_D_LCD);
    consoleDemoInit();
    consoleClear();
}

} // namespace

int main()
{
    using namespace dsi_bench;

    const MemorySnapshot processEntry = captureMemorySnapshot(true);
    consoleDemoInit();
    const RunSelection selection = selectRun();
    const bool dsiMode = isDSiMode();
    const bool oldHighClock = dsiMode && setCpuClock(true);

    consoleClear();
    iprintf("Initializing SD...\n");
    if (!fatInitDefault()) {
        iprintf("FAT initialization failed.\n");
        iprintf("bench.log cannot be written.\n");
        while (pmMainLoop()) {
            swiWaitForVBlank();
        }
        return 1;
    }
    errno = 0;
    if (mkdir("sd:/fallout1", 0777) != 0 && errno != EEXIST) {
        iprintf("Cannot create sd:/fallout1\n");
        while (pmMainLoop()) {
            swiWaitForVBlank();
        }
        return 1;
    }

    Logger log;
    if (!log.open("sd:/fallout1/bench.log", selection.appendLog)) {
        iprintf("Cannot open bench.log\n");
        while (pmMainLoop()) {
            swiWaitForVBlank();
        }
        return 1;
    }

    log.line("RUN", "BEGIN", selection.launcher);
    log.line("RUN", "FORMAT", "FALLOUT1_DSI_BENCH_V1");
    log.line("RUN", "ENV", selection.environment);
    log.line("RUN", "LAUNCHER", selection.launcher);
    log.line("RUN", "DSI_MODE", dsiMode ? 1u : 0u);
    log.line("RUN", "CPU_134MHZ_REQUESTED", dsiMode ? 1u : 0u);
    log.line("RUN", "CPU_WAS_134MHZ_BEFORE_REQUEST", oldHighClock ? 1u : 0u);
    log.line("RUN", "AUTHORITATIVE",
        std::strcmp(selection.environment, "REAL_DSI") == 0 && dsiMode
            ? "REAL_DSI_USER_ATTESTED"
            : "NO");
    log.line("RUN", "EMULATOR_TIMING_VALID_FOR_GO_NO_GO",
        std::strcmp(selection.environment, "MELONDS") == 0 ? "NO" : "NOT_APPLICABLE");
    log.line("TOOLCHAIN", "COMPILER", __VERSION__);
    log.line("TOOLCHAIN", "BUILD_DATE", __DATE__ "_" __TIME__);
    log.line("TOOLCHAIN", "GIT_REVISION", DSI_BENCH_GIT_REVISION);
    log.line("TOOLCHAIN", "TIMER_TICKS_PER_SECOND", static_cast<uint32_t>(BUS_CLOCK));
    log.line("TOOLCHAIN", "TIMER_CONVERSION", "libnds_timerTicks2usec");
    logMemorySnapshot(log, "01_PROCESS_ENTRY", "PROCESS_ENTRY", processEntry);
    logMemoryCheckpoint(log, "02_AFTER_PLATFORM_FILESYSTEM_INITIALIZATION",
        "PLATFORM_FILESYSTEM");
    logUnavailableRuntimeCheckpoints(log);

    uint32_t rectPassed = 0;
    uint32_t rectFailed = 0;
    const bool rectOk = runRectMapSelfTest(&rectPassed, &rectFailed);
    log.line("RECTMAP", "PASSED", rectPassed);
    log.line("RECTMAP", "FAILED", rectFailed);
    log.line("RECTMAP", "STATUS", rectOk ? "PASS" : "FAIL");
    log.flush();

    const bool sdOk = runSdBenchmark(log);
    logMemoryCheckpoint(log, "02B_AFTER_SD_BENCHMARK", "SD_IO");

    RenderResources resources = {};
    const bool allocationOk = allocateRenderResources(&resources);
    logMemoryCheckpoint(log,
        "04_AFTER_INDEXED_FRAMEBUFFER_AND_RENDER_STAGING",
        "FRAMEBUFFER_RENDER_STAGING");
    bool renderOk = false;
    if (allocationOk) {
        renderOk = runRenderBenchmark(log, resources);
    } else {
        log.line("RENDER", "STATUS", "FAIL_ALLOCATION");
        log.flush();
    }
    logMemoryCheckpoint(log, "04B_AFTER_RENDER_BENCHMARK", "RENDER_COMPLETE");

    restoreConsole();
    bool whiteTextureFaultsObserved = false;
    const bool inputOk = runInputSelfTest(log, &whiteTextureFaultsObserved);
    glResetTextures();
    glUnlockVRAMBank(VRAM_C);
    freeRenderResources(&resources);
    logMemoryCheckpoint(log, "13_AFTER_BENCHMARK_TEARDOWN", "BENCH_TEARDOWN");

    const bool success = dsiMode && rectOk && sdOk && allocationOk && renderOk
        && inputOk && !whiteTextureFaultsObserved;
    log.line("RUN", "STATUS", success ? "PASS" : "FAIL");
    log.line("RUN", "END", selection.launcher);
    log.flush();
    log.close();

    consoleClear();
    iprintf("Benchmark complete: %s\n\n", success ? "PASS" : "FAIL");
    iprintf("Log: sd:/fallout1/bench.log\n\n");
    if (std::strcmp(selection.launcher, "DIRECT_UNLAUNCH") == 0) {
        iprintf("Now power off and run the same\n");
        iprintf(".nds from TWiLight Menu++.\n");
        iprintf("Choose A then R to append.\n");
    } else {
        iprintf("Return the single bench.log.\n");
    }
    while (pmMainLoop()) {
        swiWaitForVBlank();
    }
    return success ? 0 : 1;
}
