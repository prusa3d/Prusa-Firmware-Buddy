#include "menu_items_chamber.hpp"

#include <cinttypes>
#include <feature/chamber/chamber.hpp>
#include <img_resources.hpp>
#include <marlin/Configuration.h>
#include <marlin_client.hpp>
#include <numeric_input_config_common.hpp>
#include <gui/menu_item/menu_item_utils.hpp>

using namespace buddy;

// MI_CHAMBER_TARGET_TEMP
// ============================================
MI_CHAMBER_TARGET_TEMP::MI_CHAMBER_TARGET_TEMP(const char *label)
    : WiSpin(0, numeric_input_config::chamber_temp_with_off(), _(label), &img::enclosure_16x16) //
{
    const auto caps = chamber().capabilities();
    set_is_hidden(!caps.always_show_temperature_control && !caps.temperature_control());
}

void MI_CHAMBER_TARGET_TEMP::OnClick() {
    marlin_client::inject(GCodeLiteral("M141 S%f", value_opt().value_or(0)));
}

void MI_CHAMBER_TARGET_TEMP::Loop() {
    if (is_edited()) {
        return;
    }

    const bool temp_ctrl = chamber().capabilities().temperature_control();
    const auto new_val = (temp_ctrl ? chamber().target_temperature() : std::nullopt);

    loop_gcode_inject_menu_item(*this,
        {
            .update_enabled = true,
            .update_icon = true,
            .enabled = temp_ctrl,
        });

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
