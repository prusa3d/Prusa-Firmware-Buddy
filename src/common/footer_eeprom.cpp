/**
 * @file footer_eeprom.cpp
 * @author Radek Vana
 * @date 2021-05-20
 */

#include "footer_eeprom.hpp"
#include <config_store/store_instance.hpp>
#include <utility_extensions.hpp>
#include <config_store/old_eeprom/eeprom_v22.hpp>

namespace footer::eeprom {

Record stored_settings_as_record() {
    return {
#if FOOTER_ITEMS_PER_LINE__ > 0
        config_store().footer_setting_0.get(),
#endif
#if FOOTER_ITEMS_PER_LINE__ > 1
            config_store().footer_setting_1.get(),
#endif
#if FOOTER_ITEMS_PER_LINE__ > 2
            config_store().footer_setting_2.get(),
#endif
#if FOOTER_ITEMS_PER_LINE__ > 3
            config_store().footer_setting_3.get(),
#endif
#if FOOTER_ITEMS_PER_LINE__ > 4
            config_store().footer_setting_4.get(),
#endif
    };
}

namespace {
    /**
     * @brief load draw configuration from eeprom
     *        check validity
     *        store valid configuration if was invalid
     * @return ItemDrawCnf
     */
    ItemDrawCnf load_and_validate_draw_cnf() {
        ItemDrawCnf cnf = ItemDrawCnf(config_store().footer_draw_type.get());
        uint32_t valid = uint32_t(cnf);
        // cannot use Set - would recursively call this function
        config_store().footer_draw_type.set(valid);
        return cnf;
    }

    ItemDrawCnf &get_draw_cnf_ref() {
        static ItemDrawCnf type = load_and_validate_draw_cnf();
        return type;
    }
} // namespace

ItemDrawCnf load_item_draw_cnf() {
    return get_draw_cnf_ref();
}

changed_t set(ItemDrawCnf cnf) {
    if (get_draw_cnf_ref() == cnf) {
        return changed_t::no;
    }
    config_store().footer_draw_type.set(static_cast<uint32_t>(cnf));
    get_draw_cnf_ref() = cnf;
    return changed_t::yes;
}

changed_t set(ItemDrawType type) {
    ItemDrawCnf cnf = get_draw_cnf_ref();
    cnf.type = type;
    return set(cnf);
}

changed_t set(draw_zero_t zero) {
    ItemDrawCnf cnf = get_draw_cnf_ref();
    cnf.zero = zero;
    return set(cnf);
}

changed_t set_center_n_and_fewer(uint8_t center_n_and_fewer) {
    ItemDrawCnf cnf = get_draw_cnf_ref();
    cnf.center_n_and_fewer = center_n_and_fewer;
    return set(cnf);
}

ItemDrawType get_item_draw_type() {
    return get_draw_cnf_ref().type;
}

draw_zero_t get_item_draw_zero() {
    return get_draw_cnf_ref().zero;
}

uint8_t get_center_n_and_fewer() {
    return get_draw_cnf_ref().center_n_and_fewer;
}

#if PRINTER_IS_PRUSA_MINI() || PRINTER_IS_PRUSA_XL()
Record decode_from_old_eeprom_v22(uint32_t encoded) {
    // Only valid for XL and MINI

    static constexpr size_t min_bit_size { config_store_ns::old_eeprom::v22::value_bit_size };
    uint32_t mask = (uint32_t(1) << (min_bit_size)) - 1;
    footer::Record ret = { {} };

    // version >= v11 has trailing ones
    encoded >>= config_store_ns::old_eeprom::v22::count_of_trailing_ones;

    for (size_t i = 0; i < config_store_ns::old_eeprom::v22::count; ++i) {
        uint32_t decoded = encoded & mask;
        if (decoded >= ftrstd::to_underlying(config_store_ns::old_eeprom::v22::FooterItems::_count)) {
            return footer::default_items; // data corrupted, return default setting
        }

        auto previous_item = static_cast<config_store_ns::old_eeprom::v22::FooterItems>(decoded);
        switch (previous_item) { // easiest way to ensure there's no mistake is the most boring one - a switch case
        case config_store_ns::old_eeprom::v22::FooterItems::Nozzle:
            ret[i] = footer::Item::nozzle;
            break;
        case config_store_ns::old_eeprom::v22::FooterItems::Bed:
            ret[i] = footer::Item::bed;
            break;
        case config_store_ns::old_eeprom::v22::FooterItems::Filament:
            ret[i] = footer::Item::filament;
            break;
        case config_store_ns::old_eeprom::v22::FooterItems::FSValue:
            ret[i] = footer::Item::f_s_value;
            break;
        case config_store_ns::old_eeprom::v22::FooterItems::FSensor:
            ret[i] = footer::Item::f_sensor;
            break;
        case config_store_ns::old_eeprom::v22::FooterItems::Speed:
            ret[i] = footer::Item::speed;
            break;
        case config_store_ns::old_eeprom::v22::FooterItems::AxisX:
            ret[i] = footer::Item::axis_x;
            break;
        case config_store_ns::old_eeprom::v22::FooterItems::AxisY:
            ret[i] = footer::Item::axis_y;
            break;
        case config_store_ns::old_eeprom::v22::FooterItems::AxisZ:
            ret[i] = footer::Item::axis_z;
            break;
        case config_store_ns::old_eeprom::v22::FooterItems::ZHeight:
            ret[i] = footer::Item::z_height;
            break;
        case config_store_ns::old_eeprom::v22::FooterItems::PrintFan:
            ret[i] = footer::Item::print_fan;
            break;
        case config_store_ns::old_eeprom::v22::FooterItems::HeatbreakFan:
            ret[i] = footer::Item::heatbreak_fan;
            break;
        case config_store_ns::old_eeprom::v22::FooterItems::Heatbreak:
            ret[i] = footer::Item::heatbreak_temp;
            break;
    #if PRINTER_IS_PRUSA_MINI()
        case config_store_ns::old_eeprom::v22::FooterItems::LiveZ:
            ret[i] = footer::Item::live_z;
            break;
        case config_store_ns::old_eeprom::v22::FooterItems::Sheets:
            ret[i] = footer::Item::sheets;
            break;
    #endif
    #if PRINTER_IS_PRUSA_XL()
        case config_store_ns::old_eeprom::v22::FooterItems::CurrentTool:
            ret[i] = footer::Item::current_tool;
            break;
        case config_store_ns::old_eeprom::v22::FooterItems::AllNozzles:
            ret[i] = footer::Item::all_nozzles;
            break;
        case config_store_ns::old_eeprom::v22::FooterItems::FSensorSide:
            ret[i] = footer::Item::f_sensor_side;
            break;
    #endif
        case config_store_ns::old_eeprom::v22::FooterItems::None:
        case config_store_ns::old_eeprom::v22::FooterItems::_count:
        default: // better to set the footer as missing than anything else in case of a random value
            ret[i] = footer::Item::none;
            break;
        }
        encoded >>= min_bit_size;
    }
    return ret;
}
#endif

/// This is just a check to make it harder to corrupt eeprom
static_assert(ftrstd::to_underlying(Item::none) == 0
        && ftrstd::to_underlying(Item::nozzle) == 1
        && ftrstd::to_underlying(Item::bed) == 2
        && ftrstd::to_underlying(Item::filament) == 3
        && ftrstd::to_underlying(Item::f_s_value) == 4
        && ftrstd::to_underlying(Item::f_sensor) == 5
        && ftrstd::to_underlying(Item::speed) == 6
        && ftrstd::to_underlying(Item::axis_x) == 7
        && ftrstd::to_underlying(Item::axis_y) == 8
        && ftrstd::to_underlying(Item::axis_z) == 9
        && ftrstd::to_underlying(Item::z_height) == 10
        && ftrstd::to_underlying(Item::print_fan) == 11
        && ftrstd::to_underlying(Item::heatbreak_fan) == 12
        && ftrstd::to_underlying(Item::input_shaper_x) == 13
        && ftrstd::to_underlying(Item::input_shaper_y) == 14
        && ftrstd::to_underlying(Item::live_z) == 15
        && ftrstd::to_underlying(Item::heatbreak_temp) == 16
        && ftrstd::to_underlying(Item::sheets) == 17
        && ftrstd::to_underlying(Item::finda) == 18
        && ftrstd::to_underlying(Item::current_tool) == 19
        && ftrstd::to_underlying(Item::all_nozzles) == 20
        && ftrstd::to_underlying(Item::f_sensor_side) == 21
        && ftrstd::to_underlying(Item::nozzle_diameter) == 22
        && ftrstd::to_underlying(Item::nozzle_pwm) == 23
        && ftrstd::to_underlying(Item::enclosure_temp) == 24
        && true, // So that we don't have to move the ',' around
    "Numbers assigned to items should never change and always be available (not ifdefed)!!");

static_assert(ftrstd::to_underlying(Item::_count) == 25, "When adding a new item, increment this counter and add it to the static_assert above");

} // namespace footer::eeprom
