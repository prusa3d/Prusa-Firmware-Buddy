/**
 * @file
 */

#include "screen_menu_modularbed.hpp"
#include "marlin_client.hpp"
#include <config_store/store_instance.hpp>

/**********************************************************************************************/
// MI_HEAT_ENTIRE_BED
MI_HEAT_ENTIRE_BED::MI_HEAT_ENTIRE_BED()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().heat_entire_bed.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_HEAT_ENTIRE_BED::OnChange([[maybe_unused]] size_t old_index) {
    config_store().heat_entire_bed.set(value());
    if (value()) {
        marlin_client::gcode("M556 A"); // enable all bedlets now
    }
}

MI_INFO_MODULAR_BED_MCU_TEMPERATURE::MI_INFO_MODULAR_BED_MCU_TEMPERATURE()
    : WI_TEMP_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
