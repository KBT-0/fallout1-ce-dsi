#include "asset_dat.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

void be32(std::vector<unsigned char>& bytes, unsigned value)
{
    bytes.push_back(static_cast<unsigned char>(value >> 24));
    bytes.push_back(static_cast<unsigned char>(value >> 16));
    bytes.push_back(static_cast<unsigned char>(value >> 8));
    bytes.push_back(static_cast<unsigned char>(value));
}

void patchBe32(std::vector<unsigned char>& bytes, size_t position, unsigned value)
{
    bytes[position] = static_cast<unsigned char>(value >> 24);
    bytes[position + 1] = static_cast<unsigned char>(value >> 16);
    bytes[position + 2] = static_cast<unsigned char>(value >> 8);
    bytes[position + 3] = static_cast<unsigned char>(value);
}

void name(std::vector<unsigned char>& bytes, const char* value)
{
    const size_t length = std::strlen(value);
    assert(length <= 255);
    bytes.push_back(static_cast<unsigned char>(length));
    bytes.insert(bytes.end(), value, value + length);
}

void dictionaryHeader(std::vector<unsigned char>& bytes, unsigned count,
    unsigned dataSize)
{
    be32(bytes, count);
    be32(bytes, count);
    be32(bytes, dataSize);
    be32(bytes, 0);
}

} // namespace

int main()
{
    std::vector<unsigned char> archive;
    dictionaryHeader(archive, 1, 0);
    name(archive, "DATA");
    dictionaryHeader(archive, 2, 16);

    name(archive, "compressed.bin");
    be32(archive, 16);
    const size_t compressedOffsetPosition = archive.size();
    be32(archive, 0);
    be32(archive, 5);
    be32(archive, 6);

    name(archive, "raw.bin");
    be32(archive, 32);
    const size_t rawOffsetPosition = archive.size();
    be32(archive, 0);
    be32(archive, 4);
    be32(archive, 0);

    patchBe32(archive, compressedOffsetPosition, archive.size());
    archive.push_back(0xff);
    archive.insert(archive.end(), { 'h', 'e', 'l', 'l', 'o' });
    patchBe32(archive, rawOffsetPosition, archive.size());
    archive.insert(archive.end(), { 1, 2, 3, 4 });

    char path[] = "/tmp/dsi-asset-dat.XXXXXX";
    const int descriptor = mkstemp(path);
    assert(descriptor >= 0);
    FILE* output = fdopen(descriptor, "wb");
    assert(output != nullptr);
    assert(std::fwrite(archive.data(), 1, archive.size(), output) == archive.size());
    assert(std::fclose(output) == 0);

    dsi_bench::DatArchive dat;
    std::string error;
    assert(dat.open(path, &error));
    assert(dat.entries().size() == 2);
    const dsi_bench::DatEntry* compressed = dat.find("data\\COMPRESSED.BIN");
    const dsi_bench::DatEntry* raw = dat.find("DATA/raw.bin");
    assert(compressed != nullptr);
    assert(raw != nullptr);

    std::vector<unsigned char> loaded;
    uint32_t loadedHash = 0;
    uint32_t streamingHash = 0;
    assert(dat.read(*compressed, &loaded, &loadedHash));
    assert(std::string(loaded.begin(), loaded.end()) == "hello");
    assert(dat.hash(*compressed, &streamingHash));
    assert(loadedHash == streamingHash);
    assert(dat.read(*raw, &loaded, &loadedHash));
    assert((loaded == std::vector<unsigned char> { 1, 2, 3, 4 }));
    assert(dat.hash(*raw, &streamingHash));
    assert(loadedHash == streamingHash);

    uint32_t archiveHash = 0;
    uint64_t archiveSize = 0;
    assert(dsi_bench::hashFile(path, &archiveHash, &archiveSize));
    assert(archiveHash != 0);
    assert(archiveSize == archive.size());
    assert(unlink(path) == 0);
    std::puts("DSi asset DAT self-test: PASS");
    return 0;
}
