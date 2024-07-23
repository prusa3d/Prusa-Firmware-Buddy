/**
 * @file footer_def.hpp
 * @author Radek Vana
 * @brief basic footer definition
 * @date 2021-05-20
 */

#pragma once
#include "printers.h"
#include <cstddef> //size_t
#include <guiconfig/guiconfig.h>
#include <array>
#include <option/has_side_fsensor.h>
#include <option/has_mmu2.h>
#include <option/has_sheet_profiles.h>
#include "i18n.h"
#include <bsod.h>
#include <device/board.h>

// sadly this must be macros, it is used in preprocessor
#if (defined(PRINTER_TYPE) && PRINTER_IS_PRUSA_MINI()) || HAS_MOCK_DISPLAY()
    #define FOOTER_HAS_LIVE_Z
    #define FOOTER_LINES__          2
    #define FOOTER_ITEMS_PER_LINE__ 3
#else
    #if (defined(PRINTER_TYPE) && PRINTER_IS_PRUSA_XL())
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
 * !!!
 - Do NOT:
   - change item values (would corrupt eeprom) - Add new items to the end
   - #ifdef any items (otherwise there could easily happen to be inconsistencies across printers that are difficult to fix (impossible without a migration)) - see disabled_items below on how to do it instead
   - leave gaps in the numbering (makes it simpler to iterate through) - currently relied upon by screen_menu_footer_settings
 */
enum class Item : uint8_t { // stored in eeprom, must fit to footer::eeprom::value_bit_size
    none = 0,
    nozzle = 1,
    bed = 2,
    filament = 3,
    f_s_value = 4,
    f_sensor = 5,
    speed = 6,
    axis_x = 7,
    axis_y = 8,
    axis_z = 9,
    z_height = 10,
    print_fan = 11,
    heatbreak_fan = 12,
    input_shaper_x = 13,
    input_shaper_y = 14,
    live_z = 15,
    heatbreak_temp = 16,
    sheets = 17,
    finda = 18,
    current_tool = 19,
    all_nozzles = 20,
    f_sensor_side = 21,
    nozzle_diameter = 22,
    nozzle_pwm = 23,
    enclosure_temp = 24,
    _count,
};

/**
 * @brief Holds all items that are not be available for specific configurations - see Item brief for why this exists
 * Note: if it were to happen that a specific configuration would have no items, ifdef it so that instead of std::to_array there's just plain std::array<Item,0>
 */
inline constexpr std::array disabled_items {
#if not defined(FOOTER_HAS_LIVE_Z)
    Item::live_z,
#endif
#if !HAS_SHEET_PROFILES()
        Item::sheets,
#endif
#if not HAS_MMU2()
        Item::finda,
#endif
#if PRINTER_IS_PRUSA_MINI() || PRINTER_IS_PRUSA_MK3_5()
        Item::heatbreak_temp,
#endif
#if not defined(FOOTER_HAS_TOOL_NR)
        Item::current_tool,
        Item::all_nozzles,
#endif
#if not _DEBUG
        Item::input_shaper_x,
        Item::input_shaper_y,
        Item::f_s_value,
#endif
#if not HAS_SIDE_FSENSOR()
        Item::f_sensor_side,
#endif
#if not XL_ENCLOSURE_SUPPORT()
        Item::enclosure_temp,
#endif
};

consteval bool all_disabled_items_are_unique() {
    auto items_copy = disabled_items;
    std::ranges::sort(items_copy);
    return std::ranges::adjacent_find(items_copy) == std::end(items_copy);
}

static_assert(all_disabled_items_are_unique(), "All disabled items must be unique");

/**
 * @brief Get name of an item.
 * @param item get name of this item
 * @return name of the item
 */
constexpr const char *to_string(Item item) {
    switch (item) {
    case Item::nozzle:
        return N_("Nozzle");
    case Item::nozzle_diameter:
        return N_("Nozzle diameter");
    case Item::nozzle_pwm:
        return N_("Nozzle PWM");
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
#if PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_MINI()
        return N_("Hotend Fan");
#else
        return N_("Heatbreak Fan");
#endif
    case Item::input_shaper_x:
        return N_("Input Shaper X");
    case Item::input_shaper_y:
        return N_("Input Shaper Y");
    case Item::live_z:
#if defined(FOOTER_HAS_LIVE_Z)
        return N_("Live Z");
#else
        break;
#endif
    case Item::heatbreak_temp:
        return N_("Heatbreak");
    case Item::sheets:
#if HAS_SHEET_PROFILES()
        return N_("Sheets");
#else
        break;
#endif
    case Item::finda:
#if HAS_MMU2()
        return N_("Finda");
#else
        break;
#endif
    case Item::current_tool:
#if defined(FOOTER_HAS_TOOL_NR)
        return N_("Current tool");
#else
        break;
#endif
    case Item::all_nozzles:
#if defined(FOOTER_HAS_TOOL_NR)
        return N_("All nozzles");
#else
        break;
#endif
    case Item::f_sensor_side:
#if HAS_SIDE_FSENSOR()
        return N_("FSensor side");
#else
        break;
#endif /*HAS_SIDE_FSENSOR()*/
    case Item::enclosure_temp:
#if XL_ENCLOSURE_SUPPORT()
        return N_("Enclosure temperature");
#else
        break;
#endif
    case Item::none:
        return N_("None");
    case Item::_count:
        break;
    }
    bsod("Nonexistent footer item");
    return "";
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
    if (lhs.type != rhs.type) {
        return false;
    }
    if (lhs.zero != rhs.zero) {
        return false;
    }
    if (lhs.center_n_and_fewer != rhs.center_n_and_fewer) {
        return false;
    }
    return true;
}

constexpr bool operator!=(ItemDrawCnf lhs, ItemDrawCnf rhs) {
    return !(lhs == rhs);
}

} // namespace footer
