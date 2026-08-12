#include "sd_bench.h"

#include "memory_probe.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace dsi_bench {

constexpr size_t kSdFileBytes = 4 * 1024 * 1024;
constexpr size_t kMaximumBlockBytes = 512 * 1024;
constexpr int kOpenCloseIterations = 64;
constexpr int kRandomReadIterations = 256;
constexpr const char* kTemporaryPath = "sd:/fallout1/bench.tmp";

static void logTransfer(Logger& log, const char* name, size_t bytes,
    uint32_t ticks, uint32_t operations)
{
    char key[96];
    const uint32_t usec = timerTicks2usec(ticks);
    const uint64_t kibPerSecond = usec == 0 ? 0
        : (static_cast<uint64_t>(bytes) * 1000000ull) / (usec * 1024ull);
    std::snprintf(key, sizeof(key), "%s.BYTES", name);
    log.line64("SD", key, bytes);
    std::snprintf(key, sizeof(key), "%s.OPERATIONS", name);
    log.line("SD", key, operations);
    std::snprintf(key, sizeof(key), "%s.TICKS", name);
    log.line("SD", key, ticks);
    std::snprintf(key, sizeof(key), "%s.USEC", name);
    log.line("SD", key, usec);
    std::snprintf(key, sizeof(key), "%s.KIB_PER_SECOND", name);
    log.line64("SD", key, kibPerSecond);
}

static bool createTemporaryFile(Logger& log, uint8_t* buffer)
{
    for (size_t index = 0; index < kMaximumBlockBytes; ++index) {
        buffer[index] = static_cast<uint8_t>((index * 37u + (index >> 8)) & 0xffu);
    }

    FILE* file = std::fopen(kTemporaryPath, "wb");
    if (file == nullptr) {
        log.line("SD", "CREATE.STATUS", "FAIL_OPEN");
        log.line("SD", "CREATE.ERRNO", static_cast<uint32_t>(errno));
        return false;
    }

    cpuStartTiming(0);
    size_t written = 0;
    while (written < kSdFileBytes) {
        const size_t count = std::fwrite(buffer, 1, kMaximumBlockBytes, file);
        if (count != kMaximumBlockBytes) {
            std::fclose(file);
            log.line("SD", "CREATE.STATUS", "FAIL_WRITE");
            log.line64("SD", "CREATE.BYTES_WRITTEN", written + count);
            return false;
        }
        written += count;
    }
    std::fflush(file);
    const uint32_t ticks = cpuGetTiming();
    std::fclose(file);

    log.line("SD", "CREATE.STATUS", "PASS");
    logTransfer(log, "CREATE_WRITE", written, ticks,
        static_cast<uint32_t>(written / kMaximumBlockBytes));
    log.line("SD", "CREATE.CHECKSUM", checksum32(buffer, kMaximumBlockBytes));
    log.flush();
    return true;
}

static bool runSequentialRead(Logger& log, uint8_t* buffer, size_t blockBytes)
{
    FILE* file = std::fopen(kTemporaryPath, "rb");
    if (file == nullptr) {
        return false;
    }

    cpuStartTiming(0);
    size_t totalRead = 0;
    uint32_t operations = 0;
    while (totalRead < kSdFileBytes) {
        const size_t count = std::fread(buffer, 1, blockBytes, file);
        if (count == 0) {
            break;
        }
        totalRead += count;
        ++operations;
    }
    const uint32_t ticks = cpuGetTiming();
    std::fclose(file);

    char name[48];
    std::snprintf(name, sizeof(name), "SEQUENTIAL_%luK",
        static_cast<unsigned long>(blockBytes / 1024));
    logTransfer(log, name, totalRead, ticks, operations);
    char key[80];
    std::snprintf(key, sizeof(key), "%s.CHECKSUM_LAST_BLOCK", name);
    log.line("SD", key, checksum32(buffer, blockBytes));
    return totalRead == kSdFileBytes;
}

static bool runRandomReads(Logger& log, uint8_t* buffer)
{
    constexpr size_t blockBytes = 4096;
    constexpr uint32_t blockCount = kSdFileBytes / blockBytes;
    FILE* file = std::fopen(kTemporaryPath, "rb");
    if (file == nullptr) {
        return false;
    }

    uint32_t state = 0x51d51d51u;
    uint32_t checksum = 0;
    cpuStartTiming(0);
    int completed = 0;
    for (; completed < kRandomReadIterations; ++completed) {
        state = state * 1664525u + 1013904223u;
        const uint32_t block = state % blockCount;
        if (std::fseek(file, static_cast<long>(block * blockBytes), SEEK_SET) != 0) {
            break;
        }
        if (std::fread(buffer, 1, blockBytes, file) != blockBytes) {
            break;
        }
        checksum ^= checksum32(buffer, blockBytes) + block;
    }
    const uint32_t ticks = cpuGetTiming();
    std::fclose(file);
    logTransfer(log, "RANDOM_4K", static_cast<size_t>(completed) * blockBytes,
        ticks, static_cast<uint32_t>(completed));
    log.line("SD", "RANDOM_4K.CHECKSUM", checksum);
    return completed == kRandomReadIterations;
}

static bool runOpenClose(Logger& log)
{
    uint32_t completed = 0;
    cpuStartTiming(0);
    for (; completed < kOpenCloseIterations; ++completed) {
        FILE* file = std::fopen(kTemporaryPath, "rb");
        if (file == nullptr) {
            break;
        }
        std::fclose(file);
    }
    const uint32_t ticks = cpuGetTiming();
    logTransfer(log, "OPEN_CLOSE", 0, ticks, completed);
    log.line("SD", "OPEN_CLOSE.MEDIAN_NOT_RECORDED", 1);
    return completed == kOpenCloseIterations;
}

bool runSdBenchmark(Logger& log)
{
    log.status("SD benchmark...");
    uint8_t* buffer = static_cast<uint8_t*>(trackedAlloc(kMaximumBlockBytes));
    if (buffer == nullptr) {
        log.line("SD", "STATUS", "FAIL_NO_BUFFER");
        return false;
    }

    bool success = createTemporaryFile(log, buffer);
    const size_t blocks[] = { 4096, 32768, 131072, 524288 };
    if (success) {
        for (size_t blockBytes : blocks) {
            success = runSequentialRead(log, buffer, blockBytes) && success;
        }
        success = runRandomReads(log, buffer) && success;
        success = runOpenClose(log) && success;
    }

    const int removeResult = std::remove(kTemporaryPath);
    log.line("SD", "TEMP_REMOVE_STATUS", removeResult == 0 ? "PASS" : "FAIL");
    log.line("SD", "STATUS", success ? "PASS" : "FAIL");
    log.flush();
    trackedFree(buffer, kMaximumBlockBytes);
    return success;
}

} // namespace dsi_bench
