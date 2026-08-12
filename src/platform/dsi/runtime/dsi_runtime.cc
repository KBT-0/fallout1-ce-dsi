#include "dsi_runtime.h"

#include <fat.h>
#include <nds.h>

#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

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
}

} // namespace

bool dsiRuntimeInit()
{
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
    if (!isDSiMode()) {
        dsiStartupStage("NOT_DSI_MODE");
        dsiFatal("This build requires DSi mode.");
        return false;
    }
    setCpuClock(true);
    if (chdir(kOriginalRoot) != 0) {
        dsiStartupStage("ORIGINAL_CHDIR_FAILED");
        dsiFatal("Missing sd:/fallout1/original/.");
        return false;
    }
    dsiStartupStage("RUNTIME_READY_ORIGINAL_CWD");
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
    showConsole("Fallout DSi failure", message);
}

} // namespace fallout
