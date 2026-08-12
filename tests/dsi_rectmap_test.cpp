#include "rectmap.h"

#include <cstdio>

int main()
{
    uint32_t passed = 0;
    uint32_t failed = 0;
    const bool ok = dsi_bench::runRectMapSelfTest(&passed, &failed);
    std::printf("RECTMAP.PASSED=%lu\n", static_cast<unsigned long>(passed));
    std::printf("RECTMAP.FAILED=%lu\n", static_cast<unsigned long>(failed));
    return ok ? 0 : 1;
}
