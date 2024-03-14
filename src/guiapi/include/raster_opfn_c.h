/**
 * @file raster_opfn_c.h
 * @brief raster operation function constants for C
 */
#pragma once

// raster operation function constants
enum {
    ROPFN_COPY = 0x00, // copy (no operation)
    ROPFN_INVERT = 0x01, // invert
    ROPFN_SWAPBW = 0x02, // swap black-white
    ROPFN_SHADOW = 0x04, // darker colors
    ROPFN_DESATURATE = 0x10, // desaturate (color average)
};
