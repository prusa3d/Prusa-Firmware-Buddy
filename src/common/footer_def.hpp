/**
 * @file footer_def.hpp
 * @author Radek Vana
 * @brief basic footer definition
 * @date 2021-05-20
 */

#pragma once
#include "printers.h"
#include <cstddef> //size_t
#include "guiconfig.h"
#include <array>

// sadly this must be macros, it is used in preprocessor
#if (defined(PRINTER_TYPE) && PRINTER_TYPE == PRINTER_PRUSA_MINI) || defined(USE_MOCK_DISPLAY)
    #define FOOTER_HAS_LIVE_Z
    #define FOOTER_HAS_SHEETS
    #define FOOTER_LINES__          2
    #define FOOTER_ITEMS_PER_LINE__ 3
#endif

namespace footer {
static constexpr uint8_t DefaultCenterNAndFewer = FOOTER_ITEMS_PER_LINE__ - 1;

/**
 * @brief enum enlisting all footer items
 * add items to the end of enum or it will break upgrades
 * if item is added other modifications must be made:
 * - IMiFooter          in src/gui/screen_menu_footer_settings.cpp
 * - ItemUnion          in src/gui/footer/footer_item_union.hpp
 * - FooterLine::Create in src/gui/footer/footer_line.cpp
 */
enum class items : uint8_t { // stored in eeprom, must be small
    ItemNozzle,
    ItemBed,
    ItemFilament,
    ItemFSensor,
    ItemSpeed,
    ItemAxisX,
    ItemAxisY,
    ItemAxisZ,
    ItemZHeight,
    ItemPrintFan,
    ItemHeatbreakFan,
#if defined(FOOTER_HAS_LIVE_Z)
    ItemLiveZ,
#endif
#if defined(FOOTER_HAS_SHEETS)
    ItemSheets,
#endif
    count_
};

using record = std::array<items, FOOTER_ITEMS_PER_LINE__>;

/**
 * @brief default record
 */
#if FOOTER_LINES__ == 2 && FOOTER_ITEMS_PER_LINE__ == 3
static constexpr record DefaultItems = { { items::ItemSpeed,
    items::ItemZHeight,
    items::ItemFilament } };
#endif // FOOTER_LINES__ == 2 && FOOTER_ITEMS_PER_LINE__ == 3

enum class ItemDrawType : uint8_t {
    Static,            // numbers at fixed positions
    StaticLeftAligned, // numbers aligned to the left, but fix size
    Dynamic            // numbers aligned to the left, dynamic size
};
static constexpr ItemDrawType DefaultDrawType = ItemDrawType::Dynamic;

// ensure meaningfull value when flash is corrupted
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

enum class draw_zero_t : bool { no,
    yes };
static constexpr draw_zero_t DefaultDrawZero = draw_zero_t::no;

struct ItemDrawCnf {
    ItemDrawType type;
    draw_zero_t zero;
    uint8_t centerNAndFewer; // any value is safe, big numbers just disable center

    constexpr operator uint32_t() const {
        return uint32_t(type) | (uint32_t(zero) << 8) | (uint32_t(centerNAndFewer) << 16);
    }
    constexpr ItemDrawCnf(uint32_t data)
        : type(Ui8ToItemDrawType(data & 0xff))
        , zero((((data >> 8) & 0xff) == 0) ? draw_zero_t::no : draw_zero_t::yes)
        , centerNAndFewer((data >> 16) & 0xff) {
        // data was invalid, set default
        if (data != uint32_t(*this)) {
            *this = Default();
        }
    }
    constexpr ItemDrawCnf(ItemDrawType type, draw_zero_t zero, uint8_t centerNAndFewer)
        : type(type)
        , zero(zero)
        , centerNAndFewer(centerNAndFewer) {}
    static constexpr ItemDrawCnf Default() {
        return ItemDrawCnf(DefaultDrawType, DefaultDrawZero, DefaultCenterNAndFewer);
    }
};
static_assert(sizeof(ItemDrawCnf) <= 4, "invalid ctor - constexpr ItemDrawCnf(uint32_t data)");

// 4B var, better pass by value
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
