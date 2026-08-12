#pragma once

#include <cstdarg>

namespace fallout {

bool dsiRuntimeInit();
void dsiRuntimeShutdown();
void dsiStartupStage(const char* stage);
const char* dsiLastStartupStage();
void dsiLog(const char* format, ...);
void dsiLogV(const char* format, va_list args);
void dsiFatal(const char* message);

} // namespace fallout
