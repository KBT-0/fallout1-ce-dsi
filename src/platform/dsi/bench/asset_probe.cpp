#include "asset_probe.h"

#include "asset_dat.h"
#include "memory_probe.h"

#include <nds.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace dsi_bench {
namespace {

constexpr const char* kOriginalRoot = "sd:/fallout1/original";
constexpr const char* kTurkishRoot = "sd:/fallout1/turkish";
constexpr size_t kMaximumProbeAssetBytes = 4 * 1024 * 1024;

struct Dataset {
    Dataset(const char* datasetKind, const char* datasetRoot)
        : kind(datasetKind)
        , root(datasetRoot)
        , masterHash(0)
        , masterBytes(0)
        , critterHash(0)
        , critterBytes(0)
    {
    }

    const char* kind;
    std::string root;
    std::string masterPath;
    std::string critterPath;
    std::string dataPath;
    std::string configPath;
    uint32_t masterHash;
    uint64_t masterBytes;
    uint32_t critterHash;
    uint64_t critterBytes;
    DatArchive master;
    DatArchive critter;
};

struct ManifestRecord {
    std::string path;
    uint32_t hash;
    uint64_t size;
};

bool pathType(const std::string& path, bool directory)
{
    struct stat info = {};
    if (stat(path.c_str(), &info) != 0) return false;
    return directory ? S_ISDIR(info.st_mode) : S_ISREG(info.st_mode);
}

std::string findCaseVariant(const std::string& root, const char* lower,
    const char* upper, bool directory = false)
{
    std::string path = root + "/" + lower;
    if (pathType(path, directory)) return path;
    path = root + "/" + upper;
    return pathType(path, directory) ? path : std::string();
}

std::string trim(const std::string& value)
{
    size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
        ++first;
    size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
        --last;
    return value.substr(first, last - first);
}

std::string configValue(const std::string& path, const char* wantedSection,
    const char* wantedKey)
{
    if (path.empty()) return {};
    FILE* file = std::fopen(path.c_str(), "r");
    if (file == nullptr) return {};
    char line[512];
    std::string section;
    std::string result;
    while (std::fgets(line, sizeof(line), file) != nullptr) {
        std::string value = trim(line);
        if (value.empty() || value[0] == ';' || value[0] == '#') continue;
        if (value.front() == '[' && value.back() == ']') {
            section = normalizeAssetPath(value.substr(1, value.size() - 2).c_str());
            continue;
        }
        const size_t equals = value.find('=');
        if (equals == std::string::npos) continue;
        const std::string key = normalizeAssetPath(trim(value.substr(0, equals)).c_str());
        if (section == wantedSection && key == wantedKey) {
            result = trim(value.substr(equals + 1));
            break;
        }
    }
    std::fclose(file);
    return result;
}

std::string configuredPath(const Dataset& dataset, const char* key,
    const char* defaultLower, const char* defaultUpper, bool directory)
{
    std::string relative = configValue(dataset.configPath, "system", key);
    if (relative.empty())
        return findCaseVariant(dataset.root, defaultLower, defaultUpper, directory);
    relative = normalizeAssetPath(relative.c_str());
    if (relative.empty() || relative.find("..") != std::string::npos
        || relative.find(':') != std::string::npos) return {};
    const std::string path = dataset.root + "/" + relative;
    return pathType(path, directory) ? path : std::string();
}

void logDatasetKey(Logger& log, const char* kind, const char* key,
    const char* value)
{
    char fullKey[96];
    std::snprintf(fullKey, sizeof(fullKey), "%s.%s", kind, key);
    log.line("ASSET", fullKey, value);
}

void logDatasetKey(Logger& log, const char* kind, const char* key,
    uint32_t value)
{
    char fullKey[96];
    std::snprintf(fullKey, sizeof(fullKey), "%s.%s", kind, key);
    log.line("ASSET", fullKey, value);
}

bool openDataset(Dataset* dataset, Logger& log)
{
    logDatasetKey(log, dataset->kind, "ROOT", dataset->root.c_str());
    const bool rootPresent = pathType(dataset->root, true);
    logDatasetKey(log, dataset->kind, "ROOT_PRESENT", rootPresent ? "YES" : "NO");
    if (!rootPresent) return false;

    dataset->configPath = findCaseVariant(dataset->root, "fallout.cfg", "FALLOUT.CFG");
    dataset->masterPath = configuredPath(*dataset, "master_dat", "master.dat", "MASTER.DAT", false);
    dataset->critterPath = configuredPath(*dataset, "critter_dat", "critter.dat", "CRITTER.DAT", false);
    dataset->dataPath = configuredPath(*dataset, "master_patches", "data", "DATA", true);
    logDatasetKey(log, dataset->kind, "MASTER_DAT_PRESENT",
        dataset->masterPath.empty() ? "NO" : "YES");
    logDatasetKey(log, dataset->kind, "CRITTER_DAT_PRESENT",
        dataset->critterPath.empty() ? "NO" : "YES");
    logDatasetKey(log, dataset->kind, "DATA_DIRECTORY_PRESENT",
        dataset->dataPath.empty() ? "NO" : "YES");
    logDatasetKey(log, dataset->kind, "FALLOUT_CFG_PRESENT",
        dataset->configPath.empty() ? "NO_OPTIONAL_DEFAULTS_USED" : "YES");
    const std::string language = configValue(dataset->configPath, "system", "language");
    logDatasetKey(log, dataset->kind, "CONFIG_LANGUAGE",
        language.empty() ? "english(DEFAULT)" : language.c_str());
    if (dataset->masterPath.empty() || dataset->critterPath.empty()
        || dataset->dataPath.empty()) {
        logDatasetKey(log, dataset->kind, "STATUS", "FAIL_REQUIRED_PATHS");
        return false;
    }

    std::string error;
    if (!dataset->master.open(dataset->masterPath.c_str(), &error)) {
        logDatasetKey(log, dataset->kind, "MASTER_DAT_ERROR", error.c_str());
        logDatasetKey(log, dataset->kind, "STATUS", "FAIL_MASTER_DAT");
        return false;
    }
    logDatasetKey(log, dataset->kind, "MASTER_DAT_ENTRIES",
        static_cast<uint32_t>(dataset->master.entries().size()));
    if (!dataset->critter.open(dataset->critterPath.c_str(), &error)) {
        logDatasetKey(log, dataset->kind, "CRITTER_DAT_ERROR", error.c_str());
        logDatasetKey(log, dataset->kind, "STATUS", "FAIL_CRITTER_DAT");
        return false;
    }
    logDatasetKey(log, dataset->kind, "CRITTER_DAT_ENTRIES",
        static_cast<uint32_t>(dataset->critter.entries().size()));
    const bool masterIdentity = hashFile(dataset->masterPath.c_str(),
        &dataset->masterHash, &dataset->masterBytes);
    const bool critterIdentity = hashFile(dataset->critterPath.c_str(),
        &dataset->critterHash, &dataset->critterBytes);
    logDatasetKey(log, dataset->kind, "MASTER_DAT_HASH", dataset->masterHash);
    logDatasetKey(log, dataset->kind, "CRITTER_DAT_HASH", dataset->critterHash);
    char identityKey[96];
    std::snprintf(identityKey, sizeof(identityKey), "%s.MASTER_DAT_BYTES", dataset->kind);
    log.line64("ASSET", identityKey, dataset->masterBytes);
    std::snprintf(identityKey, sizeof(identityKey), "%s.CRITTER_DAT_BYTES", dataset->kind);
    log.line64("ASSET", identityKey, dataset->critterBytes);
    logDatasetKey(log, dataset->kind, "ARCHIVE_IDENTITY_STATUS",
        masterIdentity && critterIdentity ? "PASS" : "FAIL");
    if (!masterIdentity || !critterIdentity) {
        logDatasetKey(log, dataset->kind, "STATUS", "FAIL_ARCHIVE_IDENTITY_READ");
        return false;
    }
    logDatasetKey(log, dataset->kind, "CATALOG_STATUS", "PASS");
    log.flush();
    return true;
}

bool readLooseFile(const std::string& path, std::vector<uint8_t>* output)
{
    struct stat info = {};
    if (stat(path.c_str(), &info) != 0 || info.st_size < 0
        || static_cast<uint64_t>(info.st_size) > kMaximumProbeAssetBytes) {
        return false;
    }
    FILE* file = std::fopen(path.c_str(), "rb");
    if (file == nullptr) return false;
    output->resize(static_cast<size_t>(info.st_size));
    const bool ok = output->empty()
        || std::fread(output->data(), 1, output->size(), file) == output->size();
    std::fclose(file);
    if (!ok) output->clear();
    return ok;
}

bool loadLogical(const Dataset& dataset, const char* logicalPath,
    std::vector<uint8_t>* output, const char** source)
{
    std::string native = normalizeAssetPath(logicalPath);
    std::string loose = dataset.dataPath + "/" + native;
    if (readLooseFile(loose, output)) {
        if (source != nullptr) *source = "LOOSE_DATA";
        return true;
    }
    const DatEntry* entry = dataset.master.find(logicalPath);
    if (entry != nullptr && entry->size <= kMaximumProbeAssetBytes
        && dataset.master.read(*entry, output)) {
        if (source != nullptr) *source = "MASTER_DAT";
        return true;
    }
    entry = dataset.critter.find(logicalPath);
    if (entry != nullptr && entry->size <= kMaximumProbeAssetBytes
        && dataset.critter.read(*entry, output)) {
        if (source != nullptr) *source = "CRITTER_DAT";
        return true;
    }
    return false;
}

const char* firstWithSuffix(const DatArchive& archive, const char* prefix,
    const char* suffix)
{
    for (const DatEntry& entry : archive.entries()) {
        if (entry.path.compare(0, std::strlen(prefix), prefix) == 0
            && entry.path.size() >= std::strlen(suffix)
            && entry.path.compare(entry.path.size() - std::strlen(suffix),
                std::strlen(suffix), suffix)
                == 0
            && entry.size <= kMaximumProbeAssetBytes) {
            return entry.path.c_str();
        }
    }
    return nullptr;
}

bool loadAndLog(const Dataset& dataset, Logger& log, const char* category,
    const char* logicalPath, std::vector<std::vector<uint8_t>>* resident)
{
    if (logicalPath == nullptr) {
        logDatasetKey(log, dataset.kind, category, "FAIL_NOT_FOUND");
        return false;
    }
    resident->emplace_back();
    const char* source = nullptr;
    if (!loadLogical(dataset, logicalPath, &resident->back(), &source)) {
        resident->pop_back();
        logDatasetKey(log, dataset.kind, category, "FAIL_LOAD");
        return false;
    }
    char key[96];
    std::snprintf(key, sizeof(key), "%s.%s.PATH", dataset.kind, category);
    log.line("ASSET", key, logicalPath);
    std::snprintf(key, sizeof(key), "%s.%s.SOURCE", dataset.kind, category);
    log.line("ASSET", key, source);
    std::snprintf(key, sizeof(key), "%s.%s.BYTES", dataset.kind, category);
    log.line("ASSET", key, static_cast<uint32_t>(resident->back().size()));
    std::snprintf(key, sizeof(key), "%s.%s.HASH", dataset.kind, category);
    log.line("ASSET", key,
        checksum32(resident->back().data(), resident->back().size()));
    return true;
}

uint32_t readLe32(const uint8_t* bytes)
{
    return static_cast<uint32_t>(bytes[0])
        | (static_cast<uint32_t>(bytes[1]) << 8)
        | (static_cast<uint32_t>(bytes[2]) << 16)
        | (static_cast<uint32_t>(bytes[3]) << 24);
}

struct FontView {
    const std::vector<uint8_t>* file;
    uint32_t count;
    uint32_t height;
    uint32_t spacing;
    size_t bitmapOffset;
};

struct AafView {
    const std::vector<uint8_t>* file;
    uint32_t maxHeight;
    uint32_t spacing;
    size_t bitmapOffset;
};

uint16_t readBe16Bytes(const uint8_t* bytes)
{
    return static_cast<uint16_t>((bytes[0] << 8) | bytes[1]);
}

uint32_t readBe32Bytes(const uint8_t* bytes)
{
    return (static_cast<uint32_t>(bytes[0]) << 24)
        | (static_cast<uint32_t>(bytes[1]) << 16)
        | (static_cast<uint32_t>(bytes[2]) << 8) | bytes[3];
}

bool parseFont(const std::vector<uint8_t>& file, FontView* font)
{
    if (file.size() < 20) return false;
    font->file = &file;
    font->count = readLe32(&file[0]);
    font->height = readLe32(&file[4]);
    font->spacing = readLe32(&file[8]);
    font->bitmapOffset = 20 + static_cast<size_t>(font->count) * 8;
    if (font->count == 0 || font->count > 512 || font->height == 0
        || font->height > 64 || font->bitmapOffset > file.size()) return false;
    for (uint32_t index = 0; index < font->count; ++index) {
        const size_t info = 20 + index * 8;
        const uint32_t width = readLe32(&file[info]);
        const uint32_t offset = readLe32(&file[info + 4]);
        const uint64_t end = static_cast<uint64_t>(font->bitmapOffset) + offset
            + static_cast<uint64_t>((width + 7) / 8) * font->height;
        if (width > 64 || end > file.size()) return false;
    }
    return true;
}

bool glyphHasInk(const FontView& font, uint8_t code)
{
    if (code >= font.count) return false;
    const size_t info = 20 + static_cast<size_t>(code) * 8;
    const uint32_t width = readLe32(&(*font.file)[info]);
    const uint32_t offset = readLe32(&(*font.file)[info + 4]);
    const size_t bytes = static_cast<size_t>((width + 7) / 8) * font.height;
    const uint8_t* bitmap = font.file->data() + font.bitmapOffset + offset;
    for (size_t index = 0; index < bytes; ++index) {
        if (bitmap[index] != 0) return width != 0;
    }
    return false;
}

bool parseAaf(const std::vector<uint8_t>& file, AafView* font)
{
    constexpr size_t headerBytes = 12 + 256 * 8;
    if (file.size() < headerBytes || readBe32Bytes(file.data()) != 0x41414646u)
        return false;
    font->file = &file;
    font->maxHeight = readBe16Bytes(&file[4]);
    font->spacing = readBe16Bytes(&file[6]);
    font->bitmapOffset = headerBytes;
    if (font->maxHeight == 0 || font->maxHeight > 128) return false;
    for (size_t index = 0; index < 256; ++index) {
        const size_t info = 12 + index * 8;
        const uint32_t width = readBe16Bytes(&file[info]);
        const uint32_t height = readBe16Bytes(&file[info + 2]);
        const uint32_t offset = readBe32Bytes(&file[info + 4]);
        if (width > 128 || height > font->maxHeight
            || static_cast<uint64_t>(font->bitmapOffset) + offset
                    + static_cast<uint64_t>(width) * height
                > file.size())
            return false;
    }
    return true;
}

bool aafGlyphHasInk(const AafView& font, uint8_t code)
{
    const size_t info = 12 + static_cast<size_t>(code) * 8;
    const uint32_t width = readBe16Bytes(&(*font.file)[info]);
    const uint32_t height = readBe16Bytes(&(*font.file)[info + 2]);
    const uint32_t offset = readBe32Bytes(&(*font.file)[info + 4]);
    const uint8_t* bitmap = font.file->data() + font.bitmapOffset + offset;
    for (size_t index = 0; index < static_cast<size_t>(width) * height; ++index) {
        if (bitmap[index] != 0) return width != 0 && height != 0;
    }
    return false;
}

void drawGlyph(u16* screen, const FontView& font, uint8_t code, int* cursorX,
    int y, int scale)
{
    const size_t info = 20 + static_cast<size_t>(code) * 8;
    const uint32_t width = readLe32(&(*font.file)[info]);
    const uint32_t offset = readLe32(&(*font.file)[info + 4]);
    const uint32_t rowBytes = (width + 7) / 8;
    const uint8_t* bitmap = font.file->data() + font.bitmapOffset + offset;
    for (uint32_t gy = 0; gy < font.height; ++gy) {
        for (uint32_t gx = 0; gx < width; ++gx) {
            if ((bitmap[gy * rowBytes + gx / 8] & (0x80 >> (gx & 7))) == 0) continue;
            for (int sy = 0; sy < scale; ++sy) {
                for (int sx = 0; sx < scale; ++sx) {
                    const int x = *cursorX + static_cast<int>(gx) * scale + sx;
                    const int py = y + static_cast<int>(gy) * scale + sy;
                    if (x >= 0 && x < 256 && py >= 0 && py < 192)
                        screen[py * 256 + x] = RGB15(31, 31, 31) | BIT(15);
                }
            }
        }
    }
    *cursorX += static_cast<int>(width + font.spacing + 1) * scale;
}

void drawAafGlyph(u16* screen, const AafView& font, uint8_t code, int* cursorX,
    int y, int scale)
{
    const size_t info = 12 + static_cast<size_t>(code) * 8;
    const uint32_t width = readBe16Bytes(&(*font.file)[info]);
    const uint32_t height = readBe16Bytes(&(*font.file)[info + 2]);
    const uint32_t offset = readBe32Bytes(&(*font.file)[info + 4]);
    const uint8_t* bitmap = font.file->data() + font.bitmapOffset + offset;
    for (uint32_t gy = 0; gy < height; ++gy) {
        for (uint32_t gx = 0; gx < width; ++gx) {
            if (bitmap[gy * width + gx] == 0) continue;
            for (int sy = 0; sy < scale; ++sy) {
                for (int sx = 0; sx < scale; ++sx) {
                    const int x = *cursorX + static_cast<int>(gx) * scale + sx;
                    const int py = y + static_cast<int>(gy) * scale + sy;
                    if (x >= 0 && x < 256 && py >= 0 && py < 192)
                        screen[py * 256 + x] = RGB15(15, 31, 15) | BIT(15);
                }
            }
        }
    }
    *cursorX += static_cast<int>(width + font.spacing + 1) * scale;
}

uint32_t waitForAOrX()
{
    do {
        swiWaitForVBlank();
        scanKeys();
    } while (keysHeld() != 0);
    while (pmMainLoop()) {
        swiWaitForVBlank();
        scanKeys();
        const uint32_t pressed = keysDown() & (KEY_A | KEY_X);
        if (pressed != 0) return pressed;
    }
    return KEY_X;
}

bool validateTurkishGlyphs(const Dataset& dataset, Logger& log,
    bool* visualFaults)
{
    // Windows-1254 byte values used by Fallout's byte-indexed GNW fonts.
    static const uint8_t codes[] = {
        0xe7, 0xc7, 0xf0, 0xd0, 0xfd, 0xdd,
        0xf6, 0xd6, 0xfe, 0xde, 0xfc, 0xdc,
    };
    static const char* const names[] = {
        "C_CEDILLA_LOWER", "C_CEDILLA_UPPER", "G_BREVE_LOWER",
        "G_BREVE_UPPER", "I_DOTLESS_LOWER", "I_DOTTED_UPPER",
        "O_DIAERESIS_LOWER", "O_DIAERESIS_UPPER", "S_CEDILLA_LOWER",
        "S_CEDILLA_UPPER", "U_DIAERESIS_LOWER", "U_DIAERESIS_UPPER",
    };

    std::vector<uint8_t> fontBytes;
    FontView font = {};
    int fontIndex = -1;
    for (int index = 0; index < 10; ++index) {
        char path[24];
        std::snprintf(path, sizeof(path), "font%d.fon", index);
        if (loadLogical(dataset, path, &fontBytes, nullptr) && parseFont(fontBytes, &font)) {
            bool all = true;
            for (uint8_t code : codes) all = glyphHasInk(font, code) && all;
            if (all) {
                fontIndex = index;
                break;
            }
        }
    }
    std::vector<uint8_t> aafBytes;
    AafView aaf = {};
    int aafIndex = -1;
    for (int index = 0; index < 16; ++index) {
        char path[24];
        std::snprintf(path, sizeof(path), "font%d.aaf", index);
        if (loadLogical(dataset, path, &aafBytes, nullptr) && parseAaf(aafBytes, &aaf)) {
            bool all = true;
            for (uint8_t code : codes) all = aafGlyphHasInk(aaf, code) && all;
            if (all) {
                aafIndex = index;
                break;
            }
        }
    }
    log.line("TURKISH_GLYPH", "ENCODING", "WINDOWS_1254_BYTE_INDEXED");
    log.line("TURKISH_GLYPH", "FONT_INDEX",
        fontIndex >= 0 ? static_cast<uint32_t>(fontIndex) : 0xffffffffu);
    log.line("TURKISH_GLYPH", "AAF_FONT_INDEX",
        aafIndex >= 0 ? static_cast<uint32_t>(aafIndex) : 0xffffffffu);
    for (size_t index = 0; index < sizeof(codes); ++index) {
        char key[96];
        std::snprintf(key, sizeof(key), "%s.CODE", names[index]);
        log.line("TURKISH_GLYPH", key, static_cast<uint32_t>(codes[index]));
        std::snprintf(key, sizeof(key), "%s.STRUCTURAL_STATUS", names[index]);
        log.line("TURKISH_GLYPH", key,
            fontIndex >= 0 && aafIndex >= 0 && glyphHasInk(font, codes[index])
                    && aafGlyphHasInk(aaf, codes[index])
                ? "PASS_BOTH_FON_AND_AAF"
                : "FAIL_FON_OR_AAF");
    }
    log.line("TURKISH_GLYPH", "FON_STRUCTURAL_STATUS",
        fontIndex >= 0 ? "PASS" : "FAIL");
    log.line("TURKISH_GLYPH", "AAF_STRUCTURAL_STATUS",
        aafIndex >= 0 ? "PASS" : "FAIL");
    if (fontIndex < 0 || aafIndex < 0) {
        log.line("TURKISH_GLYPH", "VISUAL_STATUS", "NOT_RUN_NO_COMPLETE_FONT");
        log.line("TURKISH_GLYPH", "STATUS", "FAIL");
        return false;
    }

    consoleClear();
    iprintf("Turkish glyph check\n\n");
    iprintf("Next screen order:\n");
    iprintf("cC gG iI oO sS uU\n");
    iprintf("with Turkish marks.\n");
    iprintf("White=FON Green=AAF\n\n");
    iprintf("A = correct, X = wrong\n");
    swiWaitForVBlank();
    videoSetMode(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    const int background = bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    u16* screen = static_cast<u16*>(bgGetGfxPtr(background));
    std::fill(screen, screen + 256 * 256, RGB15(1, 1, 4) | BIT(15));
    int cursorX = 8;
    for (uint8_t code : codes) drawGlyph(screen, font, code, &cursorX, 72, 2);
    cursorX = 8;
    for (uint8_t code : codes) drawAafGlyph(screen, aaf, code, &cursorX, 120, 2);
    const uint32_t key = waitForAOrX();
    const bool fault = (key & KEY_X) != 0;
    if (visualFaults != nullptr) *visualFaults = fault;
    log.line("TURKISH_GLYPH", "VISUAL_USER_ATTESTATION", fault ? "WRONG" : "CORRECT");
    log.line("TURKISH_GLYPH", "VISUAL_STATUS", fault ? "FAIL" : "PASS");
    log.line("TURKISH_GLYPH", "STATUS", fault ? "FAIL" : "PASS");
    log.flush();
    return !fault;
}

bool runRepresentativeLoads(const Dataset& dataset, Logger& log, bool turkish,
    bool* glyphFaults)
{
    std::vector<std::vector<uint8_t>> resident;
    resident.reserve(12);
    bool ok = true;
    logMemoryCheckpoint(log, turkish ? "05T_BEFORE_TURKISH_ASSET_LOAD"
                                    : "05_BEFORE_REPRESENTATIVE_MAP_LOAD",
        turkish ? "TURKISH_ASSET_PAYLOAD" : "ORIGINAL_ASSET_PAYLOAD");
    ok = loadAndLog(dataset, log, "PALETTE", "color.pal", &resident) && ok;
    ok = loadAndLog(dataset, log, "SCRIPTS", "scripts/scripts.lst", &resident) && ok;
    ok = loadAndLog(dataset, log, "SCRIPT_PROGRAM",
             firstWithSuffix(dataset.master, "scripts/", ".int"), &resident)
        && ok;
    std::string language = configValue(dataset.configPath, "system", "language");
    if (language.empty()) language = "english";
    const std::string configuredMessage = "text/" + normalizeAssetPath(language.c_str())
        + "/game/pro_item.msg";
    std::vector<uint8_t> messageProbe;
    const char* messages = configuredMessage.c_str();
    if (!loadLogical(dataset, messages, &messageProbe, nullptr))
        messages = firstWithSuffix(dataset.master, "text/english/", ".msg");
    ok = loadAndLog(dataset, log, "MESSAGES", messages, &resident) && ok;
    const char* map = firstWithSuffix(dataset.master, "maps/", ".map");
    ok = loadAndLog(dataset, log, "REPRESENTATIVE_MAP", map, &resident) && ok;
    ok = loadAndLog(dataset, log, "CRITTER_PROTO",
             firstWithSuffix(dataset.master, "proto/critters/", ".pro"), &resident)
        && ok;
    ok = loadAndLog(dataset, log, "ITEM_PROTO",
             firstWithSuffix(dataset.master, "proto/items/", ".pro"), &resident)
        && ok;
    logMemoryCheckpoint(log, turkish ? "06T_AFTER_TURKISH_MAP_SCRIPT_MESSAGES"
                                    : "06_AFTER_MAP_OBJECTS_SCRIPTS_MESSAGES",
        "REAL_ASSET_PAYLOAD_LOWER_BOUND");
    const char* interfaceArt = firstWithSuffix(dataset.master, "art/intrface/", ".frm");
    const char* critterArt = firstWithSuffix(dataset.critter, "art/critters/", ".frm");
    ok = loadAndLog(dataset, log, "INTERFACE_ART", interfaceArt, &resident) && ok;
    ok = loadAndLog(dataset, log, "CRITTER_ART", critterArt, &resident) && ok;
    logMemoryCheckpoint(log, turkish ? "07T_AFTER_TURKISH_ART_PRELOAD"
                                    : "07_AFTER_ART_PRELOAD_SETTLES",
        "REAL_ASSET_ART_PAYLOAD_LOWER_BOUND");
    uint64_t total = 0;
    for (const auto& bytes : resident) total += bytes.size();
    char key[64];
    std::snprintf(key, sizeof(key), "%s.RESIDENT_PAYLOAD_BYTES", dataset.kind);
    log.line64("ASSET", key, total);
    logDatasetKey(log, dataset.kind, "LOAD_STATUS", ok ? "PASS" : "FAIL");
    if (turkish) ok = validateTurkishGlyphs(dataset, log, glyphFaults) && ok;
    resident.clear();
    resident.shrink_to_fit();
    logMemoryCheckpoint(log, turkish ? "12T_AFTER_TURKISH_ASSET_UNLOAD"
                                    : "12_AFTER_MAP_UNLOAD",
        "ASSET_PAYLOAD_UNLOADED");
    return ok;
}

void scanManifest(const std::string& root, const std::string& relative,
    std::vector<ManifestRecord>* records)
{
    const std::string directoryPath = relative.empty() ? root : root + "/" + relative;
    DIR* directory = opendir(directoryPath.c_str());
    if (directory == nullptr) return;
    while (dirent* entry = readdir(directory)) {
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0)
            continue;
        const std::string childRelative = relative.empty()
            ? entry->d_name
            : relative + "/" + entry->d_name;
        const std::string childPath = root + "/" + childRelative;
        struct stat info = {};
        if (stat(childPath.c_str(), &info) != 0) continue;
        if (S_ISDIR(info.st_mode)) {
            scanManifest(root, childRelative, records);
        } else if (S_ISREG(info.st_mode)) {
            const std::string normalized = normalizeAssetPath(childRelative.c_str());
            if (normalized == "master.dat" || normalized == "critter.dat") continue;
            ManifestRecord record = { normalized, 0, 0 };
            if (hashFile(childPath.c_str(), &record.hash, &record.size))
                records->push_back(record);
        }
    }
    closedir(directory);
}

void logDifference(Logger& log, const char* group, uint32_t index,
    const char* state, const std::string& path)
{
    char key[96];
    char value[384];
    std::snprintf(key, sizeof(key), "%s.%05lu", group,
        static_cast<unsigned long>(index));
    std::snprintf(value, sizeof(value), "%s:%s", state, path.c_str());
    log.line("DATASET_DIFF", key, value);
}

uint32_t compareArchive(Logger& log, const char* name, const DatArchive& original,
    uint32_t originalWhole, uint64_t originalSize, const DatArchive& turkish,
    uint32_t turkishWhole, uint64_t turkishSize)
{
    char key[96];
    std::snprintf(key, sizeof(key), "%s.ORIGINAL_HASH", name);
    log.line("DATASET_DIFF", key, originalWhole);
    std::snprintf(key, sizeof(key), "%s.TURKISH_HASH", name);
    log.line("DATASET_DIFF", key, turkishWhole);
    if (originalWhole == turkishWhole && originalSize == turkishSize) {
        std::snprintf(key, sizeof(key), "%s.STATUS", name);
        log.line("DATASET_DIFF", key, "IDENTICAL");
        return 0;
    }

    const auto& left = original.entries();
    const auto& right = turkish.entries();
    size_t li = 0;
    size_t ri = 0;
    uint32_t differences = 0;
    while (li < left.size() || ri < right.size()) {
        if ((li & 63) == 0) {
            consoleClear();
            iprintf("Diffing %s\n%lu / %lu\n", name,
                static_cast<unsigned long>(li), static_cast<unsigned long>(left.size()));
        }
        if (ri >= right.size() || (li < left.size() && left[li].path < right[ri].path)) {
            logDifference(log, name, differences++, "REMOVED", left[li++].path);
        } else if (li >= left.size() || right[ri].path < left[li].path) {
            logDifference(log, name, differences++, "ADDED", right[ri++].path);
        } else {
            uint32_t leftHash = 0;
            uint32_t rightHash = 0;
            const bool hashOk = original.hash(left[li], &leftHash)
                && turkish.hash(right[ri], &rightHash);
            if (!hashOk || left[li].size != right[ri].size || leftHash != rightHash)
                logDifference(log, name, differences++, hashOk ? "MODIFIED" : "HASH_ERROR",
                    left[li].path);
            ++li;
            ++ri;
        }
    }
    std::snprintf(key, sizeof(key), "%s.DIFFERENT_FILES", name);
    log.line("DATASET_DIFF", key, differences);
    std::snprintf(key, sizeof(key), "%s.STATUS", name);
    log.line("DATASET_DIFF", key, "DIFFERENT");
    log.flush();
    return differences;
}

uint32_t compareLoose(Logger& log, const Dataset& original, const Dataset& turkish)
{
    std::vector<ManifestRecord> left;
    std::vector<ManifestRecord> right;
    scanManifest(original.root, "", &left);
    scanManifest(turkish.root, "", &right);
    const auto byPath = [](const ManifestRecord& a, const ManifestRecord& b) {
        return a.path < b.path;
    };
    std::sort(left.begin(), left.end(), byPath);
    std::sort(right.begin(), right.end(), byPath);
    size_t li = 0;
    size_t ri = 0;
    uint32_t differences = 0;
    while (li < left.size() || ri < right.size()) {
        if (ri >= right.size() || (li < left.size() && left[li].path < right[ri].path)) {
            logDifference(log, "LOOSE", differences++, "REMOVED", left[li++].path);
        } else if (li >= left.size() || right[ri].path < left[li].path) {
            logDifference(log, "LOOSE", differences++, "ADDED", right[ri++].path);
        } else {
            if (left[li].size != right[ri].size || left[li].hash != right[ri].hash)
                logDifference(log, "LOOSE", differences++, "MODIFIED", left[li].path);
            ++li;
            ++ri;
        }
    }
    log.line("DATASET_DIFF", "LOOSE.DIFFERENT_FILES", differences);
    return differences;
}

bool hashPeSection(const std::string& path, const char* wanted, uint32_t* hash)
{
    FILE* file = std::fopen(path.c_str(), "rb");
    if (file == nullptr) return false;
    uint8_t dos[64];
    if (std::fread(dos, 1, sizeof(dos), file) != sizeof(dos)
        || dos[0] != 'M' || dos[1] != 'Z') {
        std::fclose(file);
        return false;
    }
    const uint32_t peOffset = readLe32(dos + 0x3c);
    if (std::fseek(file, peOffset, SEEK_SET) != 0) {
        std::fclose(file);
        return false;
    }
    uint8_t header[24];
    if (std::fread(header, 1, sizeof(header), file) != sizeof(header)
        || std::memcmp(header, "PE\0\0", 4) != 0) {
        std::fclose(file);
        return false;
    }
    const uint16_t sections = header[6] | (header[7] << 8);
    const uint16_t optionalBytes = header[20] | (header[21] << 8);
    if (sections > 96 || std::fseek(file, optionalBytes, SEEK_CUR) != 0) {
        std::fclose(file);
        return false;
    }
    for (uint16_t index = 0; index < sections; ++index) {
        uint8_t section[40];
        if (std::fread(section, 1, sizeof(section), file) != sizeof(section)) break;
        char name[9] = {};
        std::memcpy(name, section, 8);
        if (std::strcmp(name, wanted) != 0) continue;
        const uint32_t bytes = readLe32(section + 16);
        const uint32_t offset = readLe32(section + 20);
        const long returnPosition = std::ftell(file);
        if (std::fseek(file, offset, SEEK_SET) != 0) break;
        uint32_t value = 2166136261u;
        for (uint32_t byteIndex = 0; byteIndex < bytes; ++byteIndex) {
            const int byte = std::fgetc(file);
            if (byte == EOF) {
                std::fclose(file);
                return false;
            }
            value ^= static_cast<uint8_t>(byte);
            value *= 16777619u;
        }
        *hash = value;
        std::fseek(file, returnPosition, SEEK_SET);
        std::fclose(file);
        return true;
    }
    std::fclose(file);
    return false;
}

std::string findExecutable(const Dataset& dataset)
{
    static const char* const names[] = {
        "falloutw.exe", "FALLOUTW.EXE", "fallout.exe", "FALLOUT.EXE",
    };
    for (const char* name : names) {
        const std::string path = dataset.root + "/" + name;
        if (pathType(path, false)) return path;
    }
    return {};
}

void compareExecutables(Logger& log, const Dataset& original, const Dataset& turkish)
{
    const std::string left = findExecutable(original);
    const std::string right = findExecutable(turkish);
    log.line("EXECUTABLE_DIFF", "ORIGINAL_PRESENT", left.empty() ? "NO" : "YES");
    log.line("EXECUTABLE_DIFF", "TURKISH_PRESENT", right.empty() ? "NO" : "YES");
    if (left.empty() || right.empty()) {
        log.line("EXECUTABLE_DIFF", "STATUS", "UNKNOWN_EXECUTABLE_NOT_SUPPLIED_OPTIONAL");
        log.line("EXECUTABLE_DIFF", "CE_BEHAVIOR_REQUIRED", "UNKNOWN");
        return;
    }
    uint32_t leftHash = 0;
    uint32_t rightHash = 0;
    uint64_t leftSize = 0;
    uint64_t rightSize = 0;
    hashFile(left.c_str(), &leftHash, &leftSize);
    hashFile(right.c_str(), &rightHash, &rightSize);
    log.line("EXECUTABLE_DIFF", "ORIGINAL_HASH", leftHash);
    log.line("EXECUTABLE_DIFF", "TURKISH_HASH", rightHash);
    if (leftHash == rightHash && leftSize == rightSize) {
        log.line("EXECUTABLE_DIFF", "STATUS", "IDENTICAL");
        log.line("EXECUTABLE_DIFF", "CE_BEHAVIOR_REQUIRED", "NO_EXECUTABLE_CHANGE_DETECTED");
        return;
    }
    uint32_t leftText = 0;
    uint32_t rightText = 0;
    const bool textAvailable = hashPeSection(left, ".text", &leftText)
        && hashPeSection(right, ".text", &rightText);
    log.line("EXECUTABLE_DIFF", "TEXT_SECTION_AVAILABLE", textAvailable ? "YES" : "NO");
    if (textAvailable) {
        log.line("EXECUTABLE_DIFF", "ORIGINAL_TEXT_HASH", leftText);
        log.line("EXECUTABLE_DIFF", "TURKISH_TEXT_HASH", rightText);
        log.line("EXECUTABLE_DIFF", "TEXT_SECTION_CHANGED", leftText != rightText ? "YES" : "NO");
    }
    log.line("EXECUTABLE_DIFF", "STATUS", "DIFFERENT");
    log.line("EXECUTABLE_DIFF", "CE_BEHAVIOR_REQUIRED",
        textAvailable && leftText != rightText
            ? "YES_PATCH_SPECIFIC_CODE_CHANGE_NEEDS_LOCAL_SEMANTIC_AUDIT"
            : "NO_CODE_CHANGE_DETECTED_CHECK_DATA_AND_RESOURCES");
}

bool compareBeforeTurkishLoad(Logger& log, const Dataset& original,
    const Dataset& turkish)
{
    log.line("DATASET_DIFF", "ORDER_GUARD", "BEGIN_BEFORE_TURKISH_LOAD");
    const uint32_t master = compareArchive(log, "MASTER_DAT", original.master,
        original.masterHash, original.masterBytes, turkish.master,
        turkish.masterHash, turkish.masterBytes);
    const uint32_t critter = compareArchive(log, "CRITTER_DAT", original.critter,
        original.critterHash, original.critterBytes, turkish.critter,
        turkish.critterHash, turkish.critterBytes);
    const uint32_t loose = compareLoose(log, original, turkish);
    compareExecutables(log, original, turkish);
    log.line("DATASET_DIFF", "TOTAL_DIFFERENT_FILES", master + critter + loose);
    log.line("DATASET_DIFF", "ORDER_GUARD", "COMPLETE_BEFORE_TURKISH_LOAD");
    log.line("DATASET_DIFF", "STATUS", "PASS_MANIFEST_RECORDED");
    log.flush();
    return true;
}

} // namespace

bool runAssetProbe(Logger& log, bool* turkishGlyphFaultsObserved)
{
    if (turkishGlyphFaultsObserved != nullptr) *turkishGlyphFaultsObserved = false;
    consoleClear();
    iprintf("Phase 0C real assets\nORIGINAL first...\n");
    log.line("ASSET", "POLICY", "USER_SUPPLIED_ONLY_NO_BUNDLED_GAME_ASSETS");
    log.line("ASSET", "ORIGINAL_TEST_ORDER", "FIRST");
    log.line("ASSET", "TURKISH_TEST_ORDER", "AFTER_ORIGINAL_AND_DIFF_MANIFEST");

    Dataset original("ORIGINAL", kOriginalRoot);
    if (!openDataset(&original, log)) {
        log.line("ASSET", "STATUS", "FAIL_ORIGINAL_MISSING_OR_INVALID");
        log.flush();
        consoleClear();
        iprintf("ORIGINAL assets missing.\n\n");
        iprintf("Need:\n/fallout1/original/\n");
        iprintf(" master.dat\n critter.dat\n data/\n");
        iprintf(" fallout.cfg optional\n\n");
        iprintf("Press A to continue other tests.\n");
        waitForAOrX();
        return false;
    }
    logMemoryCheckpoint(log, "03A_AFTER_ORIGINAL_ARCHIVE_CATALOGS",
        "REAL_FALLOUT_ARCHIVE_CATALOGS");
    bool success = runRepresentativeLoads(original, log, false, nullptr);
    logDatasetKey(log, "ORIGINAL", "STATUS", success ? "PASS" : "FAIL");
    if (!success) {
        log.line("ASSET", "TURKISH.STATUS", "NOT_RUN_ORIGINAL_FAILED");
        log.line("ASSET", "STATUS", "FAIL_ORIGINAL_LOAD");
        return false;
    }

    if (!pathType(kTurkishRoot, true)) {
        log.line("ASSET", "TURKISH.STATUS", "NOT_PRESENT_OPTIONAL_SEPARATE_TEST_NOT_RUN");
        log.line("ASSET", "STATUS", "PASS_ORIGINAL_ONLY");
        log.flush();
        return true;
    }

    Dataset turkish("TURKISH", kTurkishRoot);
    consoleClear();
    iprintf("ORIGINAL passed.\nIndexing TURKISH...\n");
    if (!openDataset(&turkish, log)) {
        log.line("ASSET", "STATUS", "FAIL_TURKISH_PRESENT_BUT_INVALID");
        return false;
    }
    logMemoryCheckpoint(log, "03T_AFTER_TURKISH_ARCHIVE_CATALOGS",
        "TURKISH_ARCHIVE_CATALOGS_BEFORE_LOAD");
    compareBeforeTurkishLoad(log, original, turkish);
    const bool turkishOk = runRepresentativeLoads(turkish, log, true,
        turkishGlyphFaultsObserved);
    logDatasetKey(log, "TURKISH", "STATUS", turkishOk ? "PASS" : "FAIL");
    log.line("ASSET", "STATUS", turkishOk ? "PASS_ORIGINAL_AND_TURKISH"
                                           : "FAIL_TURKISH_ONLY");
    log.flush();
    return turkishOk;
}

} // namespace dsi_bench
