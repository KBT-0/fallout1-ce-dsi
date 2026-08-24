#include "platform/dsi/runtime/dsi_runtime.h"

#include <nds.h>

u8 gHostMainVram[64 * 1024];
u8 gHostSubVram[64 * 1024];
u16 gHostMainPalette[256];
u16 gHostSubPalette[256];

namespace fallout {

void dsiLog(const char*, ...) { }
void dsiLogV(const char*, va_list) { }
void dsiStartupStage(const char*) { }
void dsiLogMemory(const char*, bool) { }
void dsiFatal(const char*) { }
const char* dsiLastStartupStage() { return ""; }

} // namespace fallout
