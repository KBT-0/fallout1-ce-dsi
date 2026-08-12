#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace dsi_bench {

struct DatEntry {
    std::string path;
    uint32_t flags;
    uint32_t offset;
    uint32_t size;
    uint32_t storedSize;
};

class DatArchive {
public:
    bool open(const char* path, std::string* error = nullptr);
    const std::string& path() const;
    const std::vector<DatEntry>& entries() const;
    const DatEntry* find(const char* logicalPath) const;
    bool read(const DatEntry& entry, std::vector<uint8_t>* output,
        uint32_t* hash = nullptr) const;
    bool hash(const DatEntry& entry, uint32_t* hash) const;

private:
    bool process(const DatEntry& entry, std::vector<uint8_t>* output,
        uint32_t* hash) const;

    std::string path_;
    std::vector<DatEntry> entries_;
};

bool hashFile(const char* path, uint32_t* hash, uint64_t* size = nullptr);
std::string normalizeAssetPath(const char* path);

} // namespace dsi_bench
