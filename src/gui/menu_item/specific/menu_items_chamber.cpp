#include "menu_items_chamber.hpp"

#include <feature/chamber/chamber.hpp>
#include <img_resources.hpp>
#include <marlin/Configuration.h>
#include <numeric_input_config_common.hpp>

using namespace buddy;

// MI_CHAMBER_TARGET_TEMP
// ============================================
MI_CHAMBER_TARGET_TEMP::MI_CHAMBER_TARGET_TEMP(const char *label)
    : WiSpin(0, numeric_input_config::chamber_temp_with_off(), _(label), &img::enclosure_16x16) {
    const auto caps = chamber().capabilities();
    set_is_hidden(!caps.always_show_temperature_control && !caps.temperature_control());
}

void MI_CHAMBER_TARGET_TEMP::OnClick() {
    chamber().set_target_temperature(value_opt());
}

void MI_CHAMBER_TARGET_TEMP::Loop() {
    if (is_edited()) {
        return;
    }

    const auto caps = chamber().capabilities();
    const bool temp_ctrl = caps.temperature_control();
    const auto new_val = (temp_ctrl ? chamber().target_temperature() : std::nullopt);

    set_enabled(temp_ctrl);
    set_value(new_val);
}

// MI_CHAMBER_TEMP
// ============================================
MI_CHAMBER_TEMP::MI_CHAMBER_TEMP(const char *label)
    : MenuItemAutoUpdatingLabel(_(label), standard_print_format::temp_c,
        [](auto) -> float { return chamber().current_temperature().value_or(NAN); }) //
{
    set_is_hidden(!chamber().capabilities().temperature_reporting);
}
