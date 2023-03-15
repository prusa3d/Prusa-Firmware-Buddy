//resource.h
#pragma once
#include <stdint.h>
#include "guiconfig.h"

enum ResourceId : uint8_t {
    IDR_FNT_SMALL,
    IDR_FNT_NORMAL,
    IDR_FNT_BIG,
    IDR_FNT_SPECIAL,
#ifdef USE_ILI9488
    IDR_FNT_LARGE,
#endif

};
