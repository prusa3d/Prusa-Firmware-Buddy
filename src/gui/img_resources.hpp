/**
 * @file img_resources.hpp
 * @brief this file is generated ...
 */

#pragma once
#include "guitypes.hpp" //img::Resource
#include "printers.h"

namespace img {

#include "png_resources.gen"

#if PRINTER_IS_PRUSA_iX

inline constexpr const Resource &printer_logo = prusa_ix_logo_121x40;

#endif
#if PRINTER_IS_PRUSA_MINI

inline constexpr const Resource &printer_logo = prusa_mini_logo_153x40;

#endif
#if PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5

    #ifndef _DEBUG
inline constexpr const Resource &printer_logo = prusa_mk4_logo_153x40;
    #else  // _DEBUG
inline constexpr const Resource &printer_logo = prusa_mk4_logo_debug_158x40;
    #endif // _DEBUG

#endif
#if PRINTER_IS_PRUSA_XL

    #ifndef _DEBUG
inline constexpr const Resource &printer_logo = prusa_xl_logo_158x40;
    #else  // _DEBUG
inline constexpr const Resource &printer_logo = prusa_xl_logo_debug_158x40;
    #endif // _DEBUG
#endif
} // namespace img
