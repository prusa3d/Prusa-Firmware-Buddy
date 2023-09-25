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
inline constexpr uint8_t default_center_n_and_fewer = FOOTER_ITEMS_PER_LINE__ - 1;

/**
 * @brief enum enlisting all footer items
 * add items to the end of enum or it will break upgrades
 * if item is added other modifications must be made:
 * - ItemUnion          in src/gui/footer/footer_item_union.hpp
 * - FooterLine::Create in src/gui/footer/footer_line.cpp
 * - to_string()        below
 */
enum class Item : uint8_t { // stored in eeprom, must fit to footer::eeprom::value_bit_size
    nozzle,
    bed,
    filament,
    f_s_value,
    f_sensor,
    speed,
    axis_x,
    axis_y,
    axis_z,
    z_height,
    print_fan,
    heatbreak_fan,
    input_shaper_x,
    input_shaper_y,
#if defined(FOOTER_HAS_LIVE_Z)
    live_z,
#endif
    heatbreak_temp,
#if defined(FOOTER_HAS_SHEETS)
    sheets,
#endif
#if HAS_MMU2()
    finda,
#endif
#if defined(FOOTER_HAS_TOOL_NR)
    current_tool,
    all_nozzles,
#endif
#if HAS_SIDE_FSENSOR()
    f_sensor_side,
#endif /*HAS_SIDE_FSENSOR()*/

    /// @note ItemNone must be last for EEPROM compatibility.
    ///  If we ever have better EEPROM system, we can move it to the beginning.
    none,
    _count
};

/**
 * @brief Get name of an item.
 * @param item get name of this item
 * @return name of the item
 */
constexpr const char *to_string(Item item) {
    switch (item) {
    case Item::nozzle:
        return N_("Nozzle");
    case Item::bed:
        return N_("Bed");
    case Item::filament:
        return N_("Filament");
    case Item::f_sensor:
        return N_("FSensor");
    case Item::f_s_value:
        return N_("FS Value");
    case Item::speed:
        return N_("Speed");
    case Item::axis_x:
        return N_("X");
    case Item::axis_y:
        return N_("Y");
    case Item::axis_z:
        return N_("Z");
    case Item::z_height:
        return N_("Z height");
    case Item::print_fan:
        return N_("Print fan");
    case Item::heatbreak_fan:
#ifdef USE_ST7789
        return N_("Hbrk fan");
#else
        return N_("Heatbreak fan");
#endif
    case Item::input_shaper_x:
        return N_("Input Shaper X");
    case Item::input_shaper_y:
        return N_("Input Shaper Y");
#if defined(FOOTER_HAS_LIVE_Z)
    case Item::live_z:
        return N_("Live Z");
#endif
    case Item::heatbreak_temp:
        return N_("Heatbreak");
#if defined(FOOTER_HAS_SHEETS)
    case Item::sheets:
        return N_("Sheets");
#endif
#if HAS_MMU2()
    case Item::finda:
        return N_("Finda");
#endif
#if defined(FOOTER_HAS_TOOL_NR)
    case Item::current_tool:
        return N_("Current tool");
    case Item::all_nozzles:
        return N_("All nozzles");
#endif
#if HAS_SIDE_FSENSOR()
    case Item::f_sensor_side:
        return N_("FSensor side");
#endif /*HAS_SIDE_FSENSOR()*/

    case Item::none:
        return N_("None");
    case Item::_count:
        break;
    }
    bsod("Nonexistent footer item");
}

using Record = std::array<Item, FOOTER_ITEMS_PER_LINE__>;

/**
 * @brief default record
 */
#if FOOTER_LINES__ == 2 && FOOTER_ITEMS_PER_LINE__ == 3
inline constexpr Record default_items = { { Item::speed,
    Item::z_height,
    Item::filament } };
#endif // FOOTER_LINES__ == 2 && FOOTER_ITEMS_PER_LINE__ == 3

#if FOOTER_LINES__ == 1 && FOOTER_ITEMS_PER_LINE__ == 5
inline constexpr Record default_items = { { Item::nozzle,
    Item::bed,
    Item::filament,
    Item::none,
    Item::none } };
#endif // FOOTER_LINES__ == 1 && FOOTER_ITEMS_PER_LINE__ == 5

enum class ItemDrawType : uint8_t {
    static_, // numbers at fixed positions
    static_left_aligned, // numbers aligned to the left, but fix size
    dynamic // numbers aligned to the left, dynamic size
};
inline constexpr ItemDrawType default_draw_type = ItemDrawType::dynamic;

// ensure meaningfull value when flash is corrupted
constexpr ItemDrawType to_item_draw_type(uint8_t data) {
    switch (data) {
    case uint8_t(ItemDrawType::static_):
        return ItemDrawType::static_;
    case uint8_t(ItemDrawType::static_left_aligned):
        return ItemDrawType::static_left_aligned;
    case uint8_t(ItemDrawType::dynamic):
        return ItemDrawType::dynamic;
    default:
        return default_draw_type;
    }
}

enum class draw_zero_t : bool { no,
    yes };
inline constexpr draw_zero_t default_draw_zero = draw_zero_t::no;

struct ItemDrawCnf {
    ItemDrawType type;
    draw_zero_t zero;
    uint8_t center_n_and_fewer; // any value is safe, big numbers just disable center

    constexpr operator uint32_t() const {
        return uint32_t(type) | (uint32_t(zero) << 8) | (uint32_t(center_n_and_fewer) << 16);
    }
    constexpr ItemDrawCnf(uint32_t data)
        : type(to_item_draw_type(data & 0xff))
        , zero((((data >> 8) & 0xff) == 0) ? draw_zero_t::no : draw_zero_t::yes)
        , center_n_and_fewer((data >> 16) & 0xff) {
        // data was invalid, set default
        if (data != uint32_t(*this)) {
            *this = get_default();
        }
    }
    constexpr ItemDrawCnf(ItemDrawType type, draw_zero_t zero, uint8_t center_n_and_fewer)
        : type(type)
        , zero(zero)
        , center_n_and_fewer(center_n_and_fewer) {}
    static constexpr ItemDrawCnf get_default() {
        return ItemDrawCnf(default_draw_type, default_draw_zero, default_center_n_and_fewer);
    }
};
static_assert(sizeof(ItemDrawCnf) <= 4, "invalid ctor - constexpr ItemDrawCnf(uint32_t data)");

// 4B var, better pass by value
constexpr bool operator==(ItemDrawCnf lhs, ItemDrawCnf rhs) {
    if (lhs.type != rhs.type)
        return false;
    if (lhs.zero != rhs.zero)
        return false;
    if (lhs.center_n_and_fewer != rhs.center_n_and_fewer)
        return false;
    return true;
}

constexpr bool operator!=(ItemDrawCnf lhs, ItemDrawCnf rhs) {
    return !(lhs == rhs);
}

} // namespace footer
