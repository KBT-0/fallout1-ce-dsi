#include "bench.h"

#include <algorithm>
#include <cstdarg>
#include <cstring>

namespace dsi_bench {

Logger::Logger()
    : file_(nullptr)
{
}

Logger::~Logger()
{
    close();
}

bool Logger::open(const char* path, bool append)
{
    close();
    file_ = std::fopen(path, append ? "a" : "w");
    return file_ != nullptr;
}

void Logger::close()
{
    if (file_ != nullptr) {
        std::fflush(file_);
        std::fclose(file_);
        file_ = nullptr;
    }
}

void Logger::flush()
{
    if (file_ != nullptr) {
        std::fflush(file_);
    }
}

bool Logger::isOpen() const
{
    return file_ != nullptr;
}

void Logger::line(const char* section, const char* key, const char* value)
{
    if (file_ != nullptr) {
        std::fprintf(file_, "%s.%s=%s\n", section, key, value);
    }
}

void Logger::line(const char* section, const char* key, uint32_t value)
{
    if (file_ != nullptr) {
        std::fprintf(file_, "%s.%s=%lu\n", section, key,
            static_cast<unsigned long>(value));
    }
}

void Logger::line64(const char* section, const char* key, uint64_t value)
{
    if (file_ != nullptr) {
        std::fprintf(file_, "%s.%s=%llu\n", section, key,
            static_cast<unsigned long long>(value));
    }
}

void Logger::status(const char* message)
{
    iprintf("%s\n", message);
}

TimingSummary summarizeTimings(uint32_t* samples, size_t count)
{
    TimingSummary result = {};
    if (samples == nullptr || count == 0) {
        return result;
    }

    std::sort(samples, samples + count);
    const size_t medianIndex = count / 2;
    size_t p95Index = (count * 95 + 99) / 100;
    if (p95Index > 0) {
        --p95Index;
    }
    if (p95Index >= count) {
        p95Index = count - 1;
    }

    result.medianTicks = samples[medianIndex];
    result.p95Ticks = samples[p95Index];
    result.worstTicks = samples[count - 1];
    result.medianUsec = timerTicks2usec(result.medianTicks);
    result.p95Usec = timerTicks2usec(result.p95Ticks);
    result.worstUsec = timerTicks2usec(result.worstTicks);
    return result;
}

uint32_t checksum32(const void* data, size_t bytes)
{
    const uint8_t* cursor = static_cast<const uint8_t*>(data);
    uint32_t hash = 2166136261u;
    for (size_t index = 0; index < bytes; ++index) {
        hash ^= cursor[index];
        hash *= 16777619u;
    }
    return hash;
}

void logTiming(Logger& log, const char* caseName, const char* metric,
    const TimingSummary& summary)
{
    char key[96];
    std::snprintf(key, sizeof(key), "%s.%s.MEDIAN_TICKS", caseName, metric);
    log.line("RENDER", key, summary.medianTicks);
    std::snprintf(key, sizeof(key), "%s.%s.P95_TICKS", caseName, metric);
    log.line("RENDER", key, summary.p95Ticks);
    std::snprintf(key, sizeof(key), "%s.%s.WORST_TICKS", caseName, metric);
    log.line("RENDER", key, summary.worstTicks);
    std::snprintf(key, sizeof(key), "%s.%s.MEDIAN_USEC", caseName, metric);
    log.line("RENDER", key, summary.medianUsec);
    std::snprintf(key, sizeof(key), "%s.%s.P95_USEC", caseName, metric);
    log.line("RENDER", key, summary.p95Usec);
    std::snprintf(key, sizeof(key), "%s.%s.WORST_USEC", caseName, metric);
    log.line("RENDER", key, summary.worstUsec);
}

} // namespace dsi_bench
