#include "menu_items_chamber.hpp"

#include <feature/chamber/chamber.hpp>
#include <img_resources.hpp>
#include <marlin/Configuration.h>

using namespace buddy;

constexpr NumericInputConfig chamber_temperature_config = {
    .max_value = 100,
    .special_value = 0,
    .unit = Unit::celsius,
};

// MI_CHAMBER_TARGET_TEMP
// ============================================
MI_CHAMBER_TARGET_TEMP::MI_CHAMBER_TARGET_TEMP(const char *label)
    : WiSpin(
        0,
        chamber_temperature_config,
        _(label),
        &img::enclosure_16x16 //
        ) //
{
}

void MI_CHAMBER_TARGET_TEMP::OnClick() {
    chamber().set_target_temperature(value() != config().special_value ? std::make_optional<buddy::Temperature>(value()) : std::nullopt);
}

void MI_CHAMBER_TARGET_TEMP::Loop() {
    if (is_edited()) {
        return;
    }

    const auto caps = chamber().capabilities();
    const bool temp_ctrl = caps.temperature_control();
    const auto new_val = (temp_ctrl ? chamber().target_temperature() : std::nullopt).value_or(*chamber_temperature_config.special_value);

    set_is_hidden(!caps.always_show_temperature_control && !temp_ctrl);
    set_is_enabled(temp_ctrl);
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
