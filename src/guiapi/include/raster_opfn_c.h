/**
 * @file raster_opfn_c.h
 * @author Radek Vana
 * @brief raster operation function constants for C
 * @date 2021-02-02
 */
#pragma once

//raster operation function constants
enum {
    ROPFN_COPY = 0x00,   //copy (no operation)
    ROPFN_INVERT = 0x01, //invert
    ROPFN_SWAPBW = 0x02, //swap black-white
    ROPFN_SHADOW = 0x04, //darker colors
};
