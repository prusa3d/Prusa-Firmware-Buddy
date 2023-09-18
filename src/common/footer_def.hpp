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
#include <option/has_side_fsensor.h>
#include <option/has_mmu2.h>
#include "i18n.h"
#include <bsod.h>

// sadly this must be macros, it is used in preprocessor
#if (defined(PRINTER_TYPE) && PRINTER_IS_PRUSA_MINI) || defined(USE_MOCK_DISPLAY)
    #define FOOTER_HAS_LIVE_Z
    #define FOOTER_HAS_SHEETS
    #define FOOTER_LINES__          2
    #define FOOTER_ITEMS_PER_LINE__ 3
#else // if defined(USE_ILI9488)
    #if (defined(PRINTER_TYPE) && PRINTER_IS_PRUSA_XL)
        #define FOOTER_HAS_TOOL_NR
    #endif
    #define FOOTER_LINES__          1
    #define FOOTER_ITEMS_PER_LINE__ 5
#endif

namespace footer {
inline constexpr uint8_t DefaultCenterNAndFewer = FOOTER_ITEMS_PER_LINE__ - 1;

/**
 * @brief enum enlisting all footer items
 * add items to the end of enum or it will break upgrades
 * if item is added other modifications must be made:
 * - ItemUnion          in src/gui/footer/footer_item_union.hpp
 * - FooterLine::Create in src/gui/footer/footer_line.cpp
 * - to_string()        below
 */
enum class Item : uint8_t { // stored in eeprom, must fit to footer::eeprom::value_bit_size
    Nozzle,
    Bed,
    Filament,
    FSValue,
    FSensor,
    Speed,
    AxisX,
    AxisY,
    AxisZ,
    ZHeight,
    PrintFan,
    HeatbreakFan,
#if not PRINTER_IS_PRUSA_MINI
    InputShaperX,
    InputShaperY,
#endif
#if defined(FOOTER_HAS_LIVE_Z)
    LiveZ,
#endif
#if not PRINTER_IS_PRUSA_MINI
    Heatbreak,
#endif
#if defined(FOOTER_HAS_SHEETS)
    Sheets,
#endif
#if HAS_MMU2()
    Finda,
#endif
#if defined(FOOTER_HAS_TOOL_NR)
    CurrentTool,
    AllNozzles,
#endif
#if HAS_SIDE_FSENSOR()
    FSensorSide,
#endif /*HAS_SIDE_FSENSOR()*/

    /// @note ItemNone must be last for EEPROM compatibility.
    ///  If we ever have better EEPROM system, we can move it to the beginning.
    None,
    _count
};

/**
 * @brief Get name of an item.
 * @param item get name of this item
 * @return name of the item
 */
constexpr const char *to_string(Item item) {
    switch (item) {
    case Item::Nozzle:
        return N_("Nozzle");
    case Item::Bed:
        return N_("Bed");
    case Item::Filament:
        return N_("Filament");
    case Item::FSensor:
        return N_("FSensor");
    case Item::FSValue:
        return N_("FS Value");
    case Item::Speed:
        return N_("Speed");
    case Item::AxisX:
        return N_("X");
    case Item::AxisY:
        return N_("Y");
    case Item::AxisZ:
        return N_("Z");
    case Item::ZHeight:
        return N_("Z height");
    case Item::PrintFan:
        return N_("Print fan");
    case Item::HeatbreakFan:
#ifdef USE_ST7789
        return N_("Hbrk fan");
#else
        return N_("Heatbreak fan");
#endif
#if not PRINTER_IS_PRUSA_MINI
    case Item::InputShaperX:
        return N_("Input Shaper X");
    case Item::InputShaperY:
        return N_("Input Shaper Y");
#endif
#if defined(FOOTER_HAS_LIVE_Z)
    case Item::LiveZ:
        return N_("Live Z");
#endif
#if not PRINTER_IS_PRUSA_MINI
    case Item::Heatbreak:
        return N_("Heatbreak");
#endif
#if defined(FOOTER_HAS_SHEETS)
    case Item::Sheets:
        return N_("Sheets");
#endif
#if HAS_MMU2()
    case Item::Finda:
        return N_("Finda");
#endif
#if defined(FOOTER_HAS_TOOL_NR)
    case Item::CurrentTool:
        return N_("Current tool");
    case Item::AllNozzles:
        return N_("All nozzles");
#endif
#if HAS_SIDE_FSENSOR()
    case Item::FSensorSide:
        return N_("FSensor side");
#endif /*HAS_SIDE_FSENSOR()*/

    case Item::None:
        return N_("None");
    case Item::_count:
        break;
    }
    bsod("Nonexistent footer item");
}

using record = std::array<Item, FOOTER_ITEMS_PER_LINE__>;

/**
 * @brief default record
 */
#if FOOTER_LINES__ == 2 && FOOTER_ITEMS_PER_LINE__ == 3
static constexpr record DefaultItems = { { Item::Speed,
    Item::ZHeight,
    Item::Filament } };
#endif // FOOTER_LINES__ == 2 && FOOTER_ITEMS_PER_LINE__ == 3

#if FOOTER_LINES__ == 1 && FOOTER_ITEMS_PER_LINE__ == 5
static constexpr record DefaultItems = { { Item::Nozzle,
    Item::Bed,
    Item::Filament,
    Item::None,
    Item::None } };
#endif // FOOTER_LINES__ == 1 && FOOTER_ITEMS_PER_LINE__ == 5

enum class ItemDrawType : uint8_t {
    Static,            // numbers at fixed positions
    StaticLeftAligned, // numbers aligned to the left, but fix size
    Dynamic            // numbers aligned to the left, dynamic size
};
inline constexpr ItemDrawType DefaultDrawType = ItemDrawType::Dynamic;

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
inline constexpr draw_zero_t DefaultDrawZero = draw_zero_t::no;

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

} // namespace footer
