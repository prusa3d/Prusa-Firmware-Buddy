/**
 * @file fonts.hpp
 */

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

struct font_t {
    uint8_t w;    //char width [pixels]
    uint8_t h;    //char height [pixels]
    uint8_t bpr;  //bytes per row
    uint32_t flg; //flags
    void *pcs;    //charset data pointer
    char asc_min; //min ascii code (first character)
    char asc_max; //max ascii code (last character)
};

font_t *resource_font(ResourceId id);
