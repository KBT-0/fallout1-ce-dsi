#pragma once

#include <nds.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace dsi_bench {

constexpr int kSourceWidth = 640;
constexpr int kSourceHeight = 480;
constexpr int kScreenWidth = 256;
constexpr int kScreenHeight = 192;
constexpr int kBitmapStride = 256;
constexpr int kWarmupFrames = 30;
constexpr int kMeasuredFrames = 600;

class Logger {
public:
    Logger();
    ~Logger();

    bool open(const char* path, bool append = false);
    void close();
    void flush();
    bool isOpen() const;
    void line(const char* section, const char* key, const char* value);
    void line(const char* section, const char* key, uint32_t value);
    void line64(const char* section, const char* key, uint64_t value);
    void status(const char* message);

private:
    FILE* file_;
};

struct TimingSummary {
    uint32_t medianTicks;
    uint32_t p95Ticks;
    uint32_t worstTicks;
    uint32_t medianUsec;
    uint32_t p95Usec;
    uint32_t worstUsec;
};

TimingSummary summarizeTimings(uint32_t* samples, size_t count);
uint32_t checksum32(const void* data, size_t bytes);
void logTiming(Logger& log, const char* caseName, const char* metric,
    const TimingSummary& summary);

} // namespace dsi_bench
