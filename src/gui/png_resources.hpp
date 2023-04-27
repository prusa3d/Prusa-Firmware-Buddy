/**
 * @file png_resources.hpp
 * @brief this file is generated ...
 */

#pragma once
#include "guitypes.hpp" //png::Resource
#include "printers.h"

namespace png {

#include "png_resources.gen"

#if (PRINTER_TYPE == PRINTER_PRUSA_IXL)

inline constexpr const Resource &printer_logo = prusa_ix_logo_121x40;

#endif
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)

inline constexpr const Resource &printer_logo = prusa_mini_logo_153x40;

#endif
#if (PRINTER_TYPE == PRINTER_PRUSA_MK4)

    #ifndef _DEBUG
inline constexpr const Resource &printer_logo = prusa_mk4_logo_153x40;
    #else  // _DEBUG
inline constexpr const Resource &printer_logo = prusa_mk4_logo_debug_158x40;
    #endif // _DEBUG

#endif
#if (PRINTER_TYPE == PRINTER_PRUSA_XL)

    #ifndef _DEBUG
inline constexpr const Resource &printer_logo = prusa_xl_logo_158x40;
    #else  // _DEBUG
inline constexpr const Resource &printer_logo = prusa_xl_logo_debug_158x40;
    #endif // _DEBUG
#endif
} // namespace png
