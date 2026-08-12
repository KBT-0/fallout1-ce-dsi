#include "input_selftest.h"

#include "rectmap.h"

#include <cstdlib>

namespace dsi_bench {

static void waitForRelease()
{
    do {
        swiWaitForVBlank();
        scanKeys();
    } while (keysHeld() != 0);
}

static bool waitForKey(uint32_t key)
{
    waitForRelease();
    while (pmMainLoop()) {
        swiWaitForVBlank();
        scanKeys();
        if ((keysDown() & key) != 0) {
            return true;
        }
    }
    return false;
}

static uint32_t waitForKeys(uint32_t mask)
{
    waitForRelease();
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

bool runInputSelfTest(Logger& log, bool* whiteTextureFaultsObserved)
{
    consoleClear();
    iprintf("Input self-test\n\n");
    const struct {
        int x;
        int y;
        const char* label;
    } targets[] = {
        { 16, 16, "UPPER_LEFT" },
        { 239, 16, "UPPER_RIGHT" },
        { 16, 175, "LOWER_LEFT" },
        { 239, 175, "LOWER_RIGHT" },
    };
    const RectMap fullMap = { { 0, 0, 640, 480 }, { 0, 0, 256, 192 } };
    bool success = true;

    for (size_t index = 0; index < sizeof(targets) / sizeof(targets[0]); ++index) {
        iprintf("Touch %s\n", targets[index].label);
        if (!waitForKey(KEY_TOUCH)) {
            return false;
        }
        touchPosition touch = {};
        touchRead(&touch);
        Point mapped = {};
        const bool mappedOk = mapPoint(fullMap, { touch.px, touch.py }, &mapped);
        const bool targetOk = std::abs(touch.px - targets[index].x) <= 48
            && std::abs(touch.py - targets[index].y) <= 48;
        char key[96];
        std::snprintf(key, sizeof(key), "TOUCH_%s.X", targets[index].label);
        log.line("INPUT", key, static_cast<uint32_t>(touch.px));
        std::snprintf(key, sizeof(key), "TOUCH_%s.Y", targets[index].label);
        log.line("INPUT", key, static_cast<uint32_t>(touch.py));
        std::snprintf(key, sizeof(key), "TOUCH_%s.MAPPED_X", targets[index].label);
        log.line("INPUT", key, static_cast<uint32_t>(mapped.x));
        std::snprintf(key, sizeof(key), "TOUCH_%s.MAPPED_Y", targets[index].label);
        log.line("INPUT", key, static_cast<uint32_t>(mapped.y));
        std::snprintf(key, sizeof(key), "TOUCH_%s.STATUS", targets[index].label);
        log.line("INPUT", key, mappedOk && targetOk ? "PASS" : "FAIL");
        success = mappedOk && targetOk && success;
        waitForRelease();
    }

    const struct {
        uint32_t key;
        const char* name;
    } buttons[] = {
        { KEY_UP, "UP" }, { KEY_RIGHT, "RIGHT" }, { KEY_DOWN, "DOWN" },
        { KEY_LEFT, "LEFT" }, { KEY_A, "A" }, { KEY_B, "B" },
        { KEY_X, "X" }, { KEY_Y, "Y" }, { KEY_L, "L" }, { KEY_R, "R" },
        { KEY_START, "START" }, { KEY_SELECT, "SELECT" },
    };
    iprintf("\nPress in order:\n");
    iprintf("D-pad clockwise, A B X Y,\nL R START SELECT\n");
    for (const auto& button : buttons) {
        iprintf("%s ", button.name);
        const bool pressed = waitForKey(button.key);
        char key[64];
        std::snprintf(key, sizeof(key), "BUTTON_%s", button.name);
        log.line("INPUT", key, pressed ? "PASS" : "FAIL");
        success = pressed && success;
    }
    iprintf("\n");

    iprintf("Render visual check:\n");
    iprintf("A = no white flashes\n");
    iprintf("X = white flashes seen\n");
    const uint32_t visualKey = waitForKeys(KEY_A | KEY_X);
    const bool whiteFaults = (visualKey & KEY_X) != 0;
    if (whiteTextureFaultsObserved != nullptr) {
        *whiteTextureFaultsObserved = whiteFaults;
    }
    log.line("INPUT", "WHITE_TEXTURE_FAULTS_OBSERVED",
        whiteFaults ? "YES" : "NO");
    log.line("INPUT", "WHITE_TEXTURE_VISUAL_STATUS",
        whiteFaults ? "FAIL" : "PASS");

    log.line("INPUT", "STATUS", success ? "PASS" : "FAIL");
    log.flush();
    return success;
}

} // namespace dsi_bench
