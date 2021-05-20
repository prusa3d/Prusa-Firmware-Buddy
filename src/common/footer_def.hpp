/**
 * @file footer_def.hpp
 * @author Radek Vana
 * @brief basic footer definition
 * @date 2021-05-20
 */

#pragma once
#include "printers.h"
#include <cstddef> //size_t
#include <array>

#if defined(PRINTER_TYPE) && PRINTER_TYPE == PRINTER_PRUSA_MINI
    #define FOOTER_HAS_LIVE_Z
    #define FOOTER_HAS_SHEETS
#endif

//sadly this must be macro, it is used in preprocessor
#define FOOTER_ITEMS_PER_LINE__ 3

namespace footer {
enum class items : uint8_t { // stored in eeprom, must be small
    ItemNozzle,
    ItemBed,
    ItemFilament,
    ItemSpeed,
#if defined(FOOTER_HAS_LIVE_Z)
    ItemLiveZ,
#endif
#if defined(FOOTER_HAS_SHEETS)
    ItemSheets,
#endif
    count_
};

using record = std::array<items, FOOTER_ITEMS_PER_LINE__>;

}
