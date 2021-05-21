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

enum class ItemDrawType : uint8_t {
    Static,            // numbers at fixed positions
    StaticLeftAligned, // numbers aligned to the left, but fix size
    Dynamic            // numbers aligned to the left, dynamic size
};

enum class ItemDrawZero : bool { no,
    yes };

struct ItemDrawCnf {
    ItemDrawType type;
    ItemDrawZero zero;

    constexpr operator uint32_t() const {
        return uint32_t(type) | (uint32_t(zero) << 8);
    }
    constexpr ItemDrawCnf(uint32_t data)
        : type(ItemDrawType(data & 0xff))
        , zero(ItemDrawZero((data >> 8) & 0xff)) {}
    constexpr ItemDrawCnf(ItemDrawType type, ItemDrawZero zero)
        : type(type)
        , zero(zero) {}
};

static constexpr ItemDrawType DefaultDrawType = ItemDrawType::Dynamic;
}
