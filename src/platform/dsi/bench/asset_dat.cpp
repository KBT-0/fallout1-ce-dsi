#include "asset_dat.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace dsi_bench {
namespace {

constexpr uint32_t kFnvOffset = 2166136261u;
constexpr uint32_t kFnvPrime = 16777619u;
constexpr uint32_t kMaximumEntries = 100000;
constexpr uint32_t kMaximumName = 255;

bool readBe32(FILE* file, uint32_t* value)
{
    uint8_t bytes[4];
    if (std::fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) {
        return false;
    }
    *value = (static_cast<uint32_t>(bytes[0]) << 24)
        | (static_cast<uint32_t>(bytes[1]) << 16)
        | (static_cast<uint32_t>(bytes[2]) << 8)
        | bytes[3];
    return true;
}

bool readBe16(FILE* file, uint16_t* value)
{
    uint8_t bytes[2];
    if (std::fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) {
        return false;
    }
    *value = static_cast<uint16_t>((bytes[0] << 8) | bytes[1]);
    return true;
}

void hashByte(uint32_t* hash, uint8_t value)
{
    *hash ^= value;
    *hash *= kFnvPrime;
}

struct Sink {
    std::vector<uint8_t>* output;
    uint32_t hash;
    uint32_t produced;

    bool put(uint8_t value, uint32_t limit)
    {
        if (produced >= limit) {
            return false;
        }
        if (output != nullptr) {
            output->push_back(value);
        }
        hashByte(&hash, value);
        ++produced;
        return true;
    }
};

bool decodeLzss(FILE* file, uint32_t compressedBytes, uint32_t outputLimit,
    Sink* sink)
{
    uint8_t ring[4096];
    std::memset(ring, ' ', 4078);
    std::memset(ring + 4078, 0, sizeof(ring) - 4078);
    uint32_t ringIndex = 4078;
    uint32_t consumed = 0;

    while (consumed < compressedBytes && sink->produced < outputLimit) {
        const int control = std::fgetc(file);
        if (control == EOF) {
            return false;
        }
        ++consumed;
        for (unsigned bit = 0; bit < 8 && consumed < compressedBytes
             && sink->produced < outputLimit;
             ++bit) {
            if ((control & (1u << bit)) != 0) {
                const int value = std::fgetc(file);
                if (value == EOF) {
                    return false;
                }
                ++consumed;
                const uint8_t byte = static_cast<uint8_t>(value);
                if (!sink->put(byte, outputLimit)) {
                    return false;
                }
                ring[ringIndex] = byte;
                ringIndex = (ringIndex + 1) & 0xfff;
            } else {
                const int low = std::fgetc(file);
                const int high = std::fgetc(file);
                if (low == EOF || high == EOF || consumed + 2 > compressedBytes) {
                    return false;
                }
                consumed += 2;
                const uint32_t offset = static_cast<uint32_t>(low)
                    | ((static_cast<uint32_t>(high) & 0xf0) << 4);
                const uint32_t count = (static_cast<uint32_t>(high) & 0x0f) + 3;
                for (uint32_t index = 0; index < count
                     && sink->produced < outputLimit;
                     ++index) {
                    const uint8_t byte = ring[(offset + index) & 0xfff];
                    if (!sink->put(byte, outputLimit)) {
                        return false;
                    }
                    ring[ringIndex] = byte;
                    ringIndex = (ringIndex + 1) & 0xfff;
                }
            }
        }
    }
    return consumed == compressedBytes;
}

bool readDictionaryHeader(FILE* file, uint32_t* size, uint32_t* dataSize)
{
    uint32_t maximum;
    uint32_t ignoredPointer;
    if (!readBe32(file, size) || !readBe32(file, &maximum)
        || !readBe32(file, dataSize) || !readBe32(file, &ignoredPointer)) {
        return false;
    }
    return *size <= maximum && *size <= kMaximumEntries && *dataSize <= 1024;
}

bool readName(FILE* file, std::string* name)
{
    const int length = std::fgetc(file);
    if (length == EOF || static_cast<uint32_t>(length) > kMaximumName) {
        return false;
    }
    char buffer[kMaximumName + 1];
    if (length != 0
        && std::fread(buffer, 1, static_cast<size_t>(length), file)
            != static_cast<size_t>(length)) {
        return false;
    }
    buffer[length] = '\0';
    name->assign(buffer, static_cast<size_t>(length));
    return true;
}

bool skipBytes(FILE* file, uint32_t bytes)
{
    return bytes == 0 || std::fseek(file, static_cast<long>(bytes), SEEK_CUR) == 0;
}

} // namespace

std::string normalizeAssetPath(const char* path)
{
    std::string result = path != nullptr ? path : "";
    for (char& ch : result) {
        if (ch == '\\') {
            ch = '/';
        } else if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    while (result.size() >= 2 && result[0] == '.' && result[1] == '/') {
        result.erase(0, 2);
    }
    while (!result.empty() && result[0] == '/') {
        result.erase(0, 1);
    }
    return result;
}

bool DatArchive::open(const char* path, std::string* error)
{
    path_.clear();
    entries_.clear();
    FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        if (error != nullptr) *error = "OPEN_FAILED";
        return false;
    }

    uint32_t directoryCount;
    uint32_t rootDataSize;
    if (!readDictionaryHeader(file, &directoryCount, &rootDataSize)) {
        if (error != nullptr) *error = "INVALID_ROOT_DICTIONARY";
        std::fclose(file);
        return false;
    }

    std::vector<std::string> directories;
    directories.reserve(directoryCount);
    for (uint32_t index = 0; index < directoryCount; ++index) {
        std::string directory;
        if (!readName(file, &directory) || !skipBytes(file, rootDataSize)) {
            if (error != nullptr) *error = "TRUNCATED_ROOT_DICTIONARY";
            std::fclose(file);
            return false;
        }
        directories.push_back(normalizeAssetPath(directory.c_str()));
    }

    for (uint32_t directoryIndex = 0; directoryIndex < directoryCount;
         ++directoryIndex) {
        uint32_t fileCount;
        uint32_t entryDataSize;
        if (!readDictionaryHeader(file, &fileCount, &entryDataSize)
            || entryDataSize != 16) {
            if (error != nullptr) *error = "INVALID_FILE_DICTIONARY";
            std::fclose(file);
            return false;
        }
        entries_.reserve(entries_.size() + fileCount);
        for (uint32_t fileIndex = 0; fileIndex < fileCount; ++fileIndex) {
            std::string name;
            DatEntry entry = {};
            if (!readName(file, &name) || !readBe32(file, &entry.flags)
                || !readBe32(file, &entry.offset) || !readBe32(file, &entry.size)
                || !readBe32(file, &entry.storedSize)) {
                if (error != nullptr) *error = "TRUNCATED_FILE_DICTIONARY";
                std::fclose(file);
                return false;
            }
            entry.path = directories[directoryIndex];
            if (!entry.path.empty() && entry.path != ".") entry.path += '/';
            entry.path += name;
            entry.path = normalizeAssetPath(entry.path.c_str());
            entries_.push_back(entry);
        }
    }
    std::fclose(file);
    std::sort(entries_.begin(), entries_.end(),
        [](const DatEntry& left, const DatEntry& right) {
            return left.path < right.path;
        });
    path_ = path;
    return true;
}

const std::string& DatArchive::path() const { return path_; }
const std::vector<DatEntry>& DatArchive::entries() const { return entries_; }

const DatEntry* DatArchive::find(const char* logicalPath) const
{
    const std::string wanted = normalizeAssetPath(logicalPath);
    const auto found = std::lower_bound(entries_.begin(), entries_.end(), wanted,
        [](const DatEntry& entry, const std::string& path) {
            return entry.path < path;
        });
    return found != entries_.end() && found->path == wanted ? &*found : nullptr;
}

bool DatArchive::read(const DatEntry& entry, std::vector<uint8_t>* output,
    uint32_t* hash) const
{
    return process(entry, output, hash);
}

bool DatArchive::hash(const DatEntry& entry, uint32_t* hash) const
{
    return process(entry, nullptr, hash);
}

bool DatArchive::process(const DatEntry& entry, std::vector<uint8_t>* output,
    uint32_t* hash) const
{
    FILE* file = std::fopen(path_.c_str(), "rb");
    if (file == nullptr || std::fseek(file, static_cast<long>(entry.offset), SEEK_SET) != 0) {
        if (file != nullptr) std::fclose(file);
        return false;
    }
    if (output != nullptr) {
        output->clear();
        output->reserve(entry.size);
    }
    Sink sink = { output, kFnvOffset, 0 };
    const uint32_t mode = entry.flags == 0 ? 16 : entry.flags & 0xf0;
    bool ok = true;
    if (mode == 16) {
        ok = decodeLzss(file, entry.storedSize, entry.size, &sink);
    } else if (mode == 32) {
        for (uint32_t index = 0; index < entry.size; ++index) {
            const int value = std::fgetc(file);
            if (value == EOF || !sink.put(static_cast<uint8_t>(value), entry.size)) {
                ok = false;
                break;
            }
        }
    } else if (mode == 64) {
        while (ok && sink.produced < entry.size) {
            uint16_t chunk;
            if (!readBe16(file, &chunk)) {
                ok = false;
            } else if ((chunk & 0x8000) != 0) {
                const uint32_t bytes = chunk & 0x7fff;
                for (uint32_t index = 0; index < bytes; ++index) {
                    const int value = std::fgetc(file);
                    if (value == EOF
                        || !sink.put(static_cast<uint8_t>(value), entry.size)) {
                        ok = false;
                        break;
                    }
                }
            } else {
                ok = decodeLzss(file, chunk, entry.size, &sink);
            }
        }
    } else {
        ok = false;
    }
    std::fclose(file);
    ok = ok && sink.produced == entry.size;
    if (hash != nullptr) *hash = sink.hash;
    if (!ok && output != nullptr) output->clear();
    return ok;
}

bool hashFile(const char* path, uint32_t* hash, uint64_t* size)
{
    FILE* file = std::fopen(path, "rb");
    if (file == nullptr) return false;
    uint8_t buffer[8192];
    uint32_t value = kFnvOffset;
    uint64_t total = 0;
    for (;;) {
        const size_t count = std::fread(buffer, 1, sizeof(buffer), file);
        for (size_t index = 0; index < count; ++index) hashByte(&value, buffer[index]);
        total += count;
        if (count != sizeof(buffer)) {
            const bool ok = std::feof(file) != 0;
            std::fclose(file);
            if (!ok) return false;
            if (hash != nullptr) *hash = value;
            if (size != nullptr) *size = total;
            return true;
        }
    }
}

} // namespace dsi_bench
