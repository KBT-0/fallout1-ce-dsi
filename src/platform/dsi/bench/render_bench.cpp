#include "render_bench.h"

#include "memory_probe.h"

#include <algorithm>
#include <cstring>

namespace dsi_bench {

constexpr size_t kSourceBytes = kSourceWidth * kSourceHeight;
constexpr size_t kBitmapBytes = kBitmapStride * 256;
constexpr size_t kVisibleBytes = kScreenWidth * kScreenHeight;
constexpr size_t kTextureBytes = 256 * 256;
constexpr int kTextureCount = 6;
constexpr size_t kTextureStageBytes = kTextureCount * kTextureBytes;

struct FrameSplit {
    uint32_t staging;
    uint32_t remap;
    uint32_t upload;
    uint32_t remapBack;
    uint32_t submit;
    uint32_t total;
};

static uint32_t gStagingSamples[kMeasuredFrames];
static uint32_t gRemapSamples[kMeasuredFrames];
static uint32_t gUploadSamples[kMeasuredFrames];
static uint32_t gRemapBackSamples[kMeasuredFrames];
static uint32_t gSubmitSamples[kMeasuredFrames];
static uint32_t gTotalSamples[kMeasuredFrames];

static int gTextureIds[kTextureCount];
static void* gTexturePointers[kTextureCount];
static int gMainBg = -1;
static int gSubBg = -1;

static void initializePalette(u16* palette)
{
    for (int index = 0; index < 256; ++index) {
        const int red = (index >> 3) & 31;
        const int green = (index >> 1) & 31;
        const int blue = (index ^ (index >> 3)) & 31;
        palette[index] = RGB15(red, green, blue);
    }
}

static void initializeSource(RenderResources& resources)
{
    for (int y = 0; y < kSourceHeight; ++y) {
        for (int x = 0; x < kSourceWidth; ++x) {
            resources.source[y * kSourceWidth + x] = static_cast<uint8_t>(
                ((x >> 3) + (y >> 3) * 7 + ((x ^ y) >> 4)) & 0xff);
        }
    }
    std::memset(resources.mainStage, 0, kBitmapBytes);
    std::memset(resources.subStage, 0, kBitmapBytes);
    std::memset(resources.textureStage, 0, kTextureStageBytes);
}

bool allocateRenderResources(RenderResources* resources)
{
    if (resources == nullptr) {
        return false;
    }
    *resources = {};
    resources->source = static_cast<uint8_t*>(trackedAlloc(kSourceBytes));
    resources->mainStage = static_cast<uint8_t*>(trackedAlloc(kBitmapBytes));
    resources->subStage = static_cast<uint8_t*>(trackedAlloc(kBitmapBytes));
    resources->textureStage = static_cast<uint8_t*>(trackedAlloc(kTextureStageBytes));
    if (resources->source == nullptr || resources->mainStage == nullptr
        || resources->subStage == nullptr || resources->textureStage == nullptr) {
        freeRenderResources(resources);
        return false;
    }
    initializeSource(*resources);
    return true;
}

void freeRenderResources(RenderResources* resources)
{
    if (resources == nullptr) {
        return;
    }
    trackedFree(resources->textureStage, resources->textureStage ? kTextureStageBytes : 0);
    trackedFree(resources->subStage, resources->subStage ? kBitmapBytes : 0);
    trackedFree(resources->mainStage, resources->mainStage ? kBitmapBytes : 0);
    trackedFree(resources->source, resources->source ? kSourceBytes : 0);
    *resources = {};
}

static void mutateSource(RenderResources& resources, int frame)
{
    const int originX = (frame * 7) % (kSourceWidth - 64);
    const int originY = (frame * 3) % (kSourceHeight - 64);
    for (int y = 0; y < 64; ++y) {
        uint8_t* row = resources.source + (originY + y) * kSourceWidth + originX;
        for (int x = 0; x < 64; ++x) {
            row[x] = static_cast<uint8_t>(row[x] + frame + x + y);
        }
    }
}

static void scaleFullSource(const uint8_t* source, uint8_t* destination,
    int sourceYOffset)
{
    for (int y = 0; y < kScreenHeight; ++y) {
        const int sourceY = (y * kSourceHeight) / kScreenHeight;
        const uint8_t* sourceRow = source
            + ((sourceY + sourceYOffset) % kSourceHeight) * kSourceWidth;
        uint8_t* destinationRow = destination + y * kBitmapStride;
        for (int x = 0; x < kScreenWidth; ++x) {
            destinationRow[x] = sourceRow[(x * kSourceWidth) / kScreenWidth];
        }
    }
}

static void prepareVisibleBuffers(RenderResources& resources, int frame)
{
    for (int y = 0; y < kScreenHeight; ++y) {
        uint8_t* mainRow = resources.mainStage + y * kBitmapStride;
        uint8_t* subRow = resources.subStage + y * kBitmapStride;
        for (int x = 0; x < kScreenWidth; ++x) {
            mainRow[x] = static_cast<uint8_t>((x + y + frame) & 0xff);
            subRow[x] = static_cast<uint8_t>((x * 3 + y * 5 + frame) & 0xff);
        }
    }
}

static void updateDirty(uint8_t* destination, int width, int height, int frame)
{
    const int originX = (frame * 5) % (kScreenWidth - width + 1);
    const int originY = (frame * 3) % (kScreenHeight - height + 1);
    for (int y = 0; y < height; ++y) {
        uint8_t* row = destination + (originY + y) * kBitmapStride + originX;
        for (int x = 0; x < width; ++x) {
            row[x] = static_cast<uint8_t>(frame + x * 3 + y * 7);
        }
    }
}

static void updateDirtyAt(uint8_t* destination, int stride, int originX,
    int originY, int width, int height, int frame)
{
    for (int y = 0; y < height; ++y) {
        uint8_t* row = destination + (originY + y) * stride + originX;
        for (int x = 0; x < width; ++x) {
            row[x] = static_cast<uint8_t>(frame + x * 3 + y * 7);
        }
    }
}

static void packTextureChunks(RenderResources& resources)
{
    std::memset(resources.textureStage, 0, kTextureStageBytes);
    for (int chunkY = 0; chunkY < 2; ++chunkY) {
        for (int chunkX = 0; chunkX < 3; ++chunkX) {
            uint8_t* chunk = resources.textureStage
                + (chunkY * 3 + chunkX) * kTextureBytes;
            const int copyWidth = std::min(256, kSourceWidth - chunkX * 256);
            const int copyHeight = std::min(256, kSourceHeight - chunkY * 256);
            if (copyWidth <= 0 || copyHeight <= 0) {
                continue;
            }
            for (int y = 0; y < copyHeight; ++y) {
                std::memcpy(chunk + y * 256,
                    resources.source + (chunkY * 256 + y) * kSourceWidth
                        + chunkX * 256,
                    static_cast<size_t>(copyWidth));
            }
        }
    }
}

static void resetSamples()
{
    std::memset(gStagingSamples, 0, sizeof(gStagingSamples));
    std::memset(gRemapSamples, 0, sizeof(gRemapSamples));
    std::memset(gUploadSamples, 0, sizeof(gUploadSamples));
    std::memset(gRemapBackSamples, 0, sizeof(gRemapBackSamples));
    std::memset(gSubmitSamples, 0, sizeof(gSubmitSamples));
    std::memset(gTotalSamples, 0, sizeof(gTotalSamples));
}

static void storeFrame(int index, const FrameSplit& split)
{
    gStagingSamples[index] = split.staging;
    gRemapSamples[index] = split.remap;
    gUploadSamples[index] = split.upload;
    gRemapBackSamples[index] = split.remapBack;
    gSubmitSamples[index] = split.submit;
    gTotalSamples[index] = split.total;
}

static void logCase(Logger& log, const char* name, uint32_t textureVramBytes,
    uint32_t paletteVramBytes, uint32_t stagingBytes, uint32_t uploadBytes,
    uint32_t effectiveDivisor = 1)
{
    char key[96];
#define LOG_CASE_VALUE(suffix, value) \
    do { \
        std::snprintf(key, sizeof(key), "%s.%s", name, suffix); \
        log.line("RENDER", key, static_cast<uint32_t>(value)); \
    } while (0)
    std::snprintf(key, sizeof(key), "%s.RENDER_CASE", name);
    log.line("RENDER", key, name);
    LOG_CASE_VALUE("SOURCE_BYTES", kSourceBytes);
    LOG_CASE_VALUE("TEXTURE_VRAM_BYTES", textureVramBytes);
    LOG_CASE_VALUE("PALETTE_VRAM_BYTES", paletteVramBytes);
    LOG_CASE_VALUE("MAIN_RAM_STAGING_BYTES", stagingBytes);
    LOG_CASE_VALUE("UPLOAD_BYTES_PER_FRAME", uploadBytes);
    LOG_CASE_VALUE("WARMUP_FRAMES", kWarmupFrames);
    LOG_CASE_VALUE("MEASURED_FRAMES", kMeasuredFrames);
    LOG_CASE_VALUE("EFFECTIVE_SCREEN_DIVISOR", effectiveDivisor);

    logTiming(log, name, "STAGING",
        summarizeTimings(gStagingSamples, kMeasuredFrames));
    logTiming(log, name, "REMAP",
        summarizeTimings(gRemapSamples, kMeasuredFrames));
    logTiming(log, name, "UPLOAD",
        summarizeTimings(gUploadSamples, kMeasuredFrames));
    logTiming(log, name, "REMAP_BACK",
        summarizeTimings(gRemapBackSamples, kMeasuredFrames));
    logTiming(log, name, "SUBMIT",
        summarizeTimings(gSubmitSamples, kMeasuredFrames));
    const TimingSummary total = summarizeTimings(gTotalSamples, kMeasuredFrames);
    logTiming(log, name, "TOTAL", total);
    LOG_CASE_VALUE("FRAME_US_MEDIAN", total.medianUsec);
    LOG_CASE_VALUE("FRAME_US_P95", total.p95Usec);
    LOG_CASE_VALUE("FRAME_US_WORST", total.worstUsec);
    const uint32_t effectiveFpsMilli = total.medianUsec == 0 ? 0
        : 1000000000u / (total.medianUsec * effectiveDivisor);
    LOG_CASE_VALUE("FPS_EFFECTIVE_MILLI", effectiveFpsMilli);
    uint32_t missedVblanks = 0;
    for (uint32_t sample : gTotalSamples) {
        if (timerTicks2usec(sample) > 16667) {
            ++missedVblanks;
        }
    }
    LOG_CASE_VALUE("MISSED_VBLANKS", missedVblanks);
    LOG_CASE_VALUE("WHITE_TEXTURE_FAULTS", 0);
    std::snprintf(key, sizeof(key), "%s.WHITE_TEXTURE_FAULT_DETECTION", name);
    log.line("RENDER", key, "VISUAL_CONFIRMATION_REQUIRED");
#undef LOG_CASE_VALUE
    log.flush();
}

static bool setup2d()
{
    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_5_2D);
    vramSetPrimaryBanks(VRAM_A_MAIN_BG_0x06000000, VRAM_B_LCD,
        VRAM_C_SUB_BG_0x06200000, VRAM_D_LCD);
    gMainBg = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    gSubBg = bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    if (gMainBg < 0 || gSubBg < 0) {
        return false;
    }
    initializePalette(BG_PALETTE);
    initializePalette(BG_PALETTE_SUB);
    bgUpdate();
    return true;
}

static FrameSplit runCpuFullFrame(RenderResources& resources, int frame)
{
    FrameSplit result = {};
    swiWaitForVBlank();
    cpuStartTiming(0);
    mutateSource(resources, frame);
    scaleFullSource(resources.source, resources.mainStage, 0);
    scaleFullSource(resources.source, resources.subStage, kSourceHeight / 4);
    const uint32_t afterStaging = cpuGetTiming();
    DC_FlushRange(resources.mainStage, kBitmapBytes);
    DC_FlushRange(resources.subStage, kBitmapBytes);
    dmaCopyWords(0, resources.mainStage, bgGetGfxPtr(gMainBg), kBitmapBytes);
    dmaCopyWords(0, resources.subStage, bgGetGfxPtr(gSubBg), kBitmapBytes);
    const uint32_t afterUpload = cpuGetTiming();
    bgUpdate();
    const uint32_t afterSubmit = cpuGetTiming();
    result.staging = afterStaging;
    result.upload = afterUpload - afterStaging;
    result.submit = afterSubmit - afterUpload;
    result.total = afterSubmit;
    return result;
}

static FrameSplit run2dFullFrame(RenderResources& resources, int frame)
{
    FrameSplit result = {};
    swiWaitForVBlank();
    cpuStartTiming(0);
    prepareVisibleBuffers(resources, frame);
    const uint32_t afterStaging = cpuGetTiming();
    DC_FlushRange(resources.mainStage, kBitmapBytes);
    DC_FlushRange(resources.subStage, kBitmapBytes);
    dmaCopyWords(0, resources.mainStage, bgGetGfxPtr(gMainBg), kBitmapBytes);
    dmaCopyWords(0, resources.subStage, bgGetGfxPtr(gSubBg), kBitmapBytes);
    const uint32_t afterUpload = cpuGetTiming();
    bgUpdate();
    const uint32_t afterSubmit = cpuGetTiming();
    result.staging = afterStaging;
    result.upload = afterUpload - afterStaging;
    result.submit = afterSubmit - afterUpload;
    result.total = afterSubmit;
    return result;
}

static FrameSplit run2dDirtyFrame(RenderResources& resources, int frame,
    int width, int height)
{
    FrameSplit result = {};
    const int originX = (frame * 5) % (kScreenWidth - width + 1);
    const int originY = (frame * 3) % (kScreenHeight - height + 1);
    swiWaitForVBlank();
    cpuStartTiming(0);
    updateDirtyAt(resources.mainStage, kBitmapStride, originX, originY,
        width, height, frame);
    updateDirtyAt(resources.subStage, kBitmapStride, originX, originY,
        width, height, frame + 17);
    const uint32_t afterStaging = cpuGetTiming();
    for (int y = 0; y < height; ++y) {
        uint8_t* mainSource = resources.mainStage
            + (originY + y) * kBitmapStride + originX;
        uint8_t* subSource = resources.subStage
            + (originY + y) * kBitmapStride + originX;
        uint8_t* mainTarget = reinterpret_cast<uint8_t*>(bgGetGfxPtr(gMainBg))
            + (originY + y) * kBitmapStride + originX;
        uint8_t* subTarget = reinterpret_cast<uint8_t*>(bgGetGfxPtr(gSubBg))
            + (originY + y) * kBitmapStride + originX;
        DC_FlushRange(mainSource, width);
        DC_FlushRange(subSource, width);
        dmaCopyWords(0, mainSource, mainTarget, width);
        dmaCopyWords(0, subSource, subTarget, width);
    }
    const uint32_t afterUpload = cpuGetTiming();
    bgUpdate();
    const uint32_t afterSubmit = cpuGetTiming();
    result.staging = afterStaging;
    result.upload = afterUpload - afterStaging;
    result.submit = afterSubmit - afterUpload;
    result.total = afterSubmit;
    return result;
}

template <typename Callback>
static void runFrames(Callback callback)
{
    resetSamples();
    for (int frame = -kWarmupFrames; frame < kMeasuredFrames; ++frame) {
        const FrameSplit split = callback(frame + kWarmupFrames);
        if (frame >= 0) {
            storeFrame(frame, split);
        }
    }
}

static void drawTextureGrid(int textureCount)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
    for (int index = 0; index < textureCount; ++index) {
        const int columns = textureCount > 4 ? 3 : 2;
        const int x = index % columns;
        const int y = index / columns;
        glBindTexture(0, gTextureIds[index]);
        glBegin(GL_QUADS);
        glTexCoord2t16(inttot16(0), inttot16(0));
        glVertex3v16(inttov16(x * 2), inttov16(y * 2), 0);
        glTexCoord2t16(inttot16(256), inttot16(0));
        glVertex3v16(inttov16(x * 2 + 2), inttov16(y * 2), 0);
        glTexCoord2t16(inttot16(256), inttot16(256));
        glVertex3v16(inttov16(x * 2 + 2), inttov16(y * 2 + 2), 0);
        glTexCoord2t16(inttot16(0), inttot16(256));
        glVertex3v16(inttov16(x * 2), inttov16(y * 2 + 2), 0);
        glEnd();
    }
}

static bool setup3d(RenderResources& resources, int textureCount,
    bool reserveCaptureBanks)
{
    videoSetMode(MODE_0_3D);
    videoSetModeSub(MODE_5_2D);
    glInit();
    glResetTextures();
    if (reserveCaptureBanks) {
        vramSetPrimaryBanks(VRAM_A_TEXTURE_SLOT0, VRAM_B_TEXTURE_SLOT1,
            VRAM_C_LCD, VRAM_D_LCD);
    } else {
        vramSetPrimaryBanks(VRAM_A_TEXTURE_SLOT0, VRAM_B_TEXTURE_SLOT1,
            VRAM_C_SUB_BG_0x06200000, VRAM_D_TEXTURE_SLOT3);
        // The texture allocator spans A-D independently of the current bank
        // mapping. Reserve C explicitly so the six 64 KiB chunks land in
        // A, B and D while C remains available to the sub 2D engine.
        glLockVRAMBank(VRAM_C);
    }
    vramSetBankE(VRAM_E_TEX_PALETTE);

    glEnable(GL_TEXTURE_2D);
    glClearColor(0, 0, 0, 31);
    glClearPolyID(63);
    glClearDepth(0x7fff);
    glViewport(0, 0, 255, 191);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const int projectionWidth = textureCount > 4 ? 6 : 4;
    glOrthof32(0, inttof32(projectionWidth), inttof32(4), 0,
        inttof32(-1), inttof32(1));
    glMatrixMode(GL_MODELVIEW);

    if (!glGenTextures(textureCount, gTextureIds)) {
        return false;
    }
    u16 palette[256];
    initializePalette(palette);
    for (int index = 0; index < textureCount; ++index) {
        glBindTexture(0, gTextureIds[index]);
        if (!glTexImage2D(0, 0, GL_RGB256, TEXTURE_SIZE_256,
                TEXTURE_SIZE_256, 0, TEXGEN_TEXCOORD, nullptr)) {
            return false;
        }
        if (index == 0) {
            glColorTableEXT(0, 0, 256, 0, 0, palette);
        } else {
            glAssignColorTable(0, gTextureIds[0]);
        }
        gTexturePointers[index] = glGetTexturePointer(gTextureIds[index]);
        if (gTexturePointers[index] == nullptr) {
            return false;
        }
    }
    packTextureChunks(resources);
    const u32 saved = reserveCaptureBanks
        ? vramSetPrimaryBanks(VRAM_A_LCD, VRAM_B_LCD, VRAM_C_LCD, VRAM_D_LCD)
        : vramSetPrimaryBanks(VRAM_A_LCD, VRAM_B_LCD,
              VRAM_C_SUB_BG_0x06200000, VRAM_D_LCD);
    DC_FlushRange(resources.textureStage, textureCount * kTextureBytes);
    for (int index = 0; index < textureCount; ++index) {
        dmaCopyWords(0, resources.textureStage + index * kTextureBytes,
            gTexturePointers[index], kTextureBytes);
    }
    vramRestorePrimaryBanks(saved);
    glFlush(0);
    swiWaitForVBlank();
    return true;
}

static FrameSplit run3dFullFrame(RenderResources& resources, int frame)
{
    FrameSplit result = {};
    swiWaitForVBlank();
    cpuStartTiming(0);
    mutateSource(resources, frame);
    packTextureChunks(resources);
    const uint32_t afterStaging = cpuGetTiming();
    const u32 saved = vramSetPrimaryBanks(VRAM_A_LCD, VRAM_B_LCD,
        VRAM_C_SUB_BG_0x06200000, VRAM_D_LCD);
    const uint32_t afterRemap = cpuGetTiming();
    DC_FlushRange(resources.textureStage, kTextureStageBytes);
    for (int index = 0; index < kTextureCount; ++index) {
        dmaCopyWords(0, resources.textureStage + index * kTextureBytes,
            gTexturePointers[index], kTextureBytes);
    }
    const uint32_t afterUpload = cpuGetTiming();
    vramRestorePrimaryBanks(saved);
    const uint32_t afterRemapBack = cpuGetTiming();
    drawTextureGrid(kTextureCount);
    glFlush(0);
    const uint32_t afterSubmit = cpuGetTiming();
    result.staging = afterStaging;
    result.remap = afterRemap - afterStaging;
    result.upload = afterUpload - afterRemap;
    result.remapBack = afterRemapBack - afterUpload;
    result.submit = afterSubmit - afterRemapBack;
    result.total = afterSubmit;
    return result;
}

static FrameSplit run3dDirtyFrame(RenderResources& resources, int frame)
{
    constexpr int dirtySize = 64;
    FrameSplit result = {};
    const int originX = (frame * 5) % (256 - dirtySize + 1);
    const int originY = (frame * 3) % (256 - dirtySize + 1);
    uint8_t* chunk = resources.textureStage;
    swiWaitForVBlank();
    cpuStartTiming(0);
    updateDirtyAt(chunk, 256, originX, originY, dirtySize, dirtySize, frame);
    const uint32_t afterStaging = cpuGetTiming();
    vramSetBankA(VRAM_A_LCD);
    const uint32_t afterRemap = cpuGetTiming();
    for (int y = 0; y < dirtySize; ++y) {
        uint8_t* source = chunk + (originY + y) * 256 + originX;
        uint8_t* target = static_cast<uint8_t*>(gTexturePointers[0])
            + (originY + y) * 256 + originX;
        DC_FlushRange(source, dirtySize);
        dmaCopyWords(0, source, target, dirtySize);
    }
    const uint32_t afterUpload = cpuGetTiming();
    vramSetBankA(VRAM_A_TEXTURE_SLOT0);
    const uint32_t afterRemapBack = cpuGetTiming();
    drawTextureGrid(kTextureCount);
    glFlush(0);
    const uint32_t afterSubmit = cpuGetTiming();
    result.staging = afterStaging;
    result.remap = afterRemap - afterStaging;
    result.upload = afterUpload - afterRemap;
    result.remapBack = afterRemapBack - afterUpload;
    result.submit = afterSubmit - afterRemapBack;
    result.total = afterSubmit;
    return result;
}

static FrameSplit runHybridFrame(RenderResources& resources, int frame)
{
    constexpr int dirtySize = 64;
    FrameSplit result = {};
    const int originX = (frame * 5) % (256 - dirtySize + 1);
    const int originY = (frame * 3) % (256 - dirtySize + 1);
    swiWaitForVBlank();
    cpuStartTiming(0);
    updateDirtyAt(resources.textureStage, 256, originX, originY,
        dirtySize, dirtySize, frame);
    updateDirty(resources.subStage, 128, 64, frame + 31);
    const uint32_t afterStaging = cpuGetTiming();
    vramSetBankA(VRAM_A_LCD);
    const uint32_t afterRemap = cpuGetTiming();
    for (int y = 0; y < dirtySize; ++y) {
        uint8_t* source = resources.textureStage
            + (originY + y) * 256 + originX;
        uint8_t* target = static_cast<uint8_t*>(gTexturePointers[0])
            + (originY + y) * 256 + originX;
        DC_FlushRange(source, dirtySize);
        dmaCopyWords(0, source, target, dirtySize);
    }
    DC_FlushRange(resources.subStage, kBitmapBytes);
    dmaCopyWords(0, resources.subStage, bgGetGfxPtr(gSubBg), kBitmapBytes);
    const uint32_t afterUpload = cpuGetTiming();
    vramSetBankA(VRAM_A_TEXTURE_SLOT0);
    const uint32_t afterRemapBack = cpuGetTiming();
    drawTextureGrid(kTextureCount);
    bgUpdate();
    glFlush(0);
    const uint32_t afterSubmit = cpuGetTiming();
    result.staging = afterStaging;
    result.remap = afterRemap - afterStaging;
    result.upload = afterUpload - afterRemap;
    result.remapBack = afterRemapBack - afterUpload;
    result.submit = afterSubmit - afterRemapBack;
    result.total = afterSubmit;
    return result;
}

static void initializeCaptureSprites()
{
    oamInit(&oamSub, SpriteMapping_Bmp_2D_256, false);
    int id = 0;
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 4; ++x) {
            oamSub.oamMemory[id].attribute[0] = ATTR0_BMP | ATTR0_SQUARE | (64 * y);
            oamSub.oamMemory[id].attribute[1] = ATTR1_SIZE_64 | (64 * x);
            oamSub.oamMemory[id].attribute[2] = ATTR2_ALPHA(1)
                | (8 * 32 * y) | (8 * x);
            ++id;
        }
    }
    oamUpdate(&oamSub);
}

static FrameSplit runDual3dFrame(RenderResources& resources, int frame)
{
    static bool top;
    constexpr int dirtySize = 64;
    FrameSplit result = {};
    const int originX = (frame * 5) % (256 - dirtySize + 1);
    const int originY = (frame * 3) % (256 - dirtySize + 1);
    swiWaitForVBlank();
    cpuStartTiming(0);
    updateDirtyAt(resources.textureStage, 256, originX, originY,
        dirtySize, dirtySize, frame);
    const uint32_t afterStaging = cpuGetTiming();
    while (REG_DISPCAPCNT & DCAP_ENABLE) {
    }
    vramSetBankA(VRAM_A_LCD);
    const uint32_t afterRemap = cpuGetTiming();
    for (int y = 0; y < dirtySize; ++y) {
        uint8_t* source = resources.textureStage
            + (originY + y) * 256 + originX;
        uint8_t* target = static_cast<uint8_t*>(gTexturePointers[0])
            + (originY + y) * 256 + originX;
        DC_FlushRange(source, dirtySize);
        dmaCopyWords(0, source, target, dirtySize);
    }
    const uint32_t afterUpload = cpuGetTiming();
    vramSetBankA(VRAM_A_TEXTURE_SLOT0);
    top = !top;
    if (top) {
        lcdMainOnBottom();
        vramSetBankC(VRAM_C_LCD);
        vramSetBankD(VRAM_D_SUB_SPRITE);
        REG_DISPCAPCNT = DCAP_BANK(2) | DCAP_ENABLE | DCAP_SIZE(3);
    } else {
        lcdMainOnTop();
        vramSetBankD(VRAM_D_LCD);
        vramSetBankC(VRAM_C_SUB_BG);
        REG_DISPCAPCNT = DCAP_BANK(3) | DCAP_ENABLE | DCAP_SIZE(3);
    }
    const uint32_t afterRemapBack = cpuGetTiming();
    drawTextureGrid(4);
    glFlush(0);
    const uint32_t afterSubmit = cpuGetTiming();
    result.staging = afterStaging;
    result.remap = afterRemap - afterStaging;
    result.upload = afterUpload - afterRemap;
    result.remapBack = afterRemapBack - afterUpload;
    result.submit = afterSubmit - afterRemapBack;
    result.total = afterSubmit;
    return result;
}

bool runRenderBenchmark(Logger& log, RenderResources& resources)
{
    log.status("2D render cases...");
    if (!setup2d()) {
        log.line("RENDER", "STATUS", "FAIL_2D_INIT");
        return false;
    }

    runFrames([&](int frame) { return runCpuFullFrame(resources, frame); });
    logCase(log, "CPU_FULL", 2 * kBitmapBytes, 2 * 512,
        2 * kBitmapBytes, 2 * kBitmapBytes);
    runFrames([&](int frame) { return run2dFullFrame(resources, frame); });
    logCase(log, "2D_FULL", 2 * kBitmapBytes, 2 * 512,
        2 * kBitmapBytes, 2 * kBitmapBytes);

    struct DirtyCase {
        const char* name;
        int width;
        int height;
    };
    const DirtyCase dirtyCases[] = {
        { "2D_DIRTY_32X32", 32, 32 },
        { "2D_DIRTY_64X64", 64, 64 },
        { "2D_DIRTY_128X64", 128, 64 },
    };
    for (const DirtyCase& dirty : dirtyCases) {
        runFrames([&](int frame) {
            return run2dDirtyFrame(resources, frame, dirty.width, dirty.height);
        });
        logCase(log, dirty.name, 2 * kBitmapBytes, 2 * 512,
            2 * kBitmapBytes, 2 * dirty.width * dirty.height);
    }

    log.status("3D render cases...");
    if (!setup3d(resources, kTextureCount, false)) {
        log.line("RENDER", "STATUS", "FAIL_3D_INIT");
        return false;
    }
    gSubBg = bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    initializePalette(BG_PALETTE_SUB);
    bgUpdate();

    runFrames([&](int frame) { return run3dFullFrame(resources, frame); });
    logCase(log, "3D_FULL", kTextureStageBytes, 512,
        kTextureStageBytes, kTextureStageBytes);
    runFrames([&](int frame) { return run3dDirtyFrame(resources, frame); });
    logCase(log, "3D_DIRTY", kTextureStageBytes, 512,
        kTextureStageBytes, 64 * 64);
    runFrames([&](int frame) { return runHybridFrame(resources, frame); });
    logCase(log, "HYBRID_MAIN3D_SUB2D", kTextureStageBytes + kBitmapBytes,
        2 * 512, kTextureStageBytes + kBitmapBytes, 64 * 64 + kBitmapBytes);

    log.status("Dual-3D penalty case...");
    if (!setup3d(resources, 4, true)) {
        log.line("RENDER", "STATUS", "FAIL_DUAL3D_INIT");
        return false;
    }
    initializeCaptureSprites();
    gSubBg = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    runFrames([&](int frame) { return runDual3dFrame(resources, frame); });
    logCase(log, "DUAL3D_CAPTURE_ALTERNATING", 4 * kTextureBytes + 2 * 128 * 1024,
        512, 4 * kTextureBytes, 64 * 64, 2);

    log.line("RENDER", "INDEXED_SOURCE_FORMAT", "8BPP_256_COLOR");
    log.line("RENDER", "RGB_EXPANSION", "CAPTURE_OUTPUT_ONLY");
    log.line("RENDER", "TIMED_PATH_INCLUDES",
        "CACHE_DMA_LCDC_REMAP_REMAP_BACK_SUBMISSION");
    log.line("RENDER", "STATUS", "PASS");
    log.flush();
    return true;
}

} // namespace dsi_bench
