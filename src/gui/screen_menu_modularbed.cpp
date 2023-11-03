/**
 * @file
 */

#include "screen_menu_modularbed.hpp"
#include "marlin_client.hpp"
#include <config_store/store_instance.hpp>

/**********************************************************************************************/
// MI_HEAT_ENTIRE_BED
bool MI_HEAT_ENTIRE_BED::init_index() const {
    return config_store().heat_entire_bed.get();
}

void MI_HEAT_ENTIRE_BED::OnChange(size_t old_index) {
    config_store().heat_entire_bed.set(!old_index);
    index = !old_index;
    if (index == 1) {
        marlin_client::gcode("M556 A"); // enable all bedlets now
    }
}

MI_INFO_MODULAR_BED_MCU_TEMPERATURE::MI_INFO_MODULAR_BED_MCU_TEMPERATURE()
    : WI_TEMP_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
