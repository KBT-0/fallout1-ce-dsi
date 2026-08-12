#pragma once

#include "bench.h"

namespace dsi_bench {

// ORIGINAL is mandatory. TURKISH is optional, but if its directory is present
// it is diffed before it is loaded and must pass its own checks.
bool runAssetProbe(Logger& log, bool* turkishGlyphFaultsObserved);

} // namespace dsi_bench
