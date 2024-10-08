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

MI_CHAMBER_TARGET_TEMP::MI_CHAMBER_TARGET_TEMP(const char *label)
    : WiSpin(
        chamber().target_temperature().value_or(*chamber_temperature_config.special_value),
        chamber_temperature_config,
        _(label ?: (HAS_MINI_DISPLAY() ? N_("Target") : N_("Target Temperature"))),
        &img::enclosure_16x16 //
        ) //
{
    set_is_hidden(chamber().temperature_control() == Chamber::TemperatureControl {});
}

void MI_CHAMBER_TARGET_TEMP::OnClick() {
    chamber().set_target_temperature(value() != config().special_value ? std::make_optional<buddy::Temperature>(value()) : std::nullopt);
}
