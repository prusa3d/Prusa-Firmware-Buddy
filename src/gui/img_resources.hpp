#pragma once
#include "guitypes.hpp" //img::Resource

namespace img {

#include "qoi_resources.gen"

#ifdef UNITTESTS

#elif PRINTER_IS_PRUSA_iX()
inline constexpr const Resource &printer_logo = prusa_ix_logo_121x40;

#elif PRINTER_IS_PRUSA_MINI()
inline constexpr const Resource &printer_logo = prusa_mini_logo_153x40;

#elif PRINTER_IS_PRUSA_MK4() || PRINTER_IS_PRUSA_MK3_5()
    #ifndef _DEBUG
inline constexpr const Resource &printer_logo = prusa_mk4_logo_153x40;
    #else // _DEBUG
inline constexpr const Resource &printer_logo = prusa_mk4_logo_debug_158x40;
    #endif // _DEBUG

#elif PRINTER_IS_PRUSA_XL()
    #ifndef _DEBUG
inline constexpr const Resource &printer_logo = prusa_xl_logo_158x40;
    #else // _DEBUG
inline constexpr const Resource &printer_logo = prusa_xl_logo_debug_158x40;
    #endif // _DEBUG
#endif

#ifndef UNITTESTS
static constexpr std::array spinner_16x16_stages { &img::spinner0_16x16, &img::spinner1_16x16, &img::spinner2_16x16, &img::spinner3_16x16 };
#endif

/**
 * @brief Enable global resource file to be used.
 */
void enable_resource_file();

/**
 * @brief Get global resource file.
 * Can be used only after enable_resource_file() was called.
 * @return FILE* to resource file.
 */
FILE *get_resource_file();

} // namespace img
