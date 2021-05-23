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

static constexpr ItemDrawType DefaultDrawType = ItemDrawType::Dynamic;

//ensure meaningfull value when flash is corrupted
constexpr ItemDrawType Ui8ToItemDrawType(uint8_t data) {
    switch (data) {
    case uint8_t(ItemDrawType::Static):
        return ItemDrawType::Static;
    case uint8_t(ItemDrawType::StaticLeftAligned):
        return ItemDrawType::StaticLeftAligned;
    case uint8_t(ItemDrawType::Dynamic):
        return ItemDrawType::Dynamic;
    default:
        return DefaultDrawType;
    }
}

enum class ItemDrawZero : bool { no,
    yes };

struct ItemDrawCnf {
    ItemDrawType type;
    ItemDrawZero zero;
    uint8_t centerNAndFewer; // any value is safe, big numbers just disable center

    constexpr operator uint32_t() const {
        return uint32_t(type) | (uint32_t(zero) << 8) | (uint32_t(centerNAndFewer) << 16);
    }
    constexpr ItemDrawCnf(uint32_t data)
        : type(Ui8ToItemDrawType(data & 0xff))
        , zero((((data >> 8) & 0xff) == 0) ? ItemDrawZero::no : ItemDrawZero::yes)
        , centerNAndFewer((data >> 16) & 0xff) {}
    constexpr ItemDrawCnf(ItemDrawType type, ItemDrawZero zero, uint8_t centerNAndFewer)
        : type(type)
        , zero(zero)
        , centerNAndFewer(centerNAndFewer) {}
};
static_assert(sizeof(ItemDrawCnf) <= 4, "invalid ctor - constexpr ItemDrawCnf(uint32_t data)");

//4B var, better pass by value
constexpr bool operator==(ItemDrawCnf lhs, ItemDrawCnf rhs) {
    if (lhs.type != rhs.type)
        return false;
    if (lhs.zero != rhs.zero)
        return false;
    if (lhs.centerNAndFewer != rhs.centerNAndFewer)
        return false;
    return true;
}

constexpr bool operator!=(ItemDrawCnf lhs, ItemDrawCnf rhs) {
    return !(lhs == rhs);
}

}
