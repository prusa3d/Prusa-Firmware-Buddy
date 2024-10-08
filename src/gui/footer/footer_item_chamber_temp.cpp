#include "footer_item_chamber_temp.hpp"
#include "img_resources.hpp"

#include <feature/chamber/chamber.hpp>

using namespace buddy;

FooterItemChamberTemperature::FooterItemChamberTemperature(window_t *parent)
    : FooterItemHeater(parent, &img::enclosure_16x16, static_makeView, static_readValue) {
}

int FooterItemChamberTemperature::static_readValue() {
    const auto current = chamber().current_temperature();
    const auto target = chamber().target_temperature();
    const HeatState state = [&] {
        static constexpr auto tolerance = 2;

        const auto temp_control = chamber().temperature_control();

        if (!current.has_value() || !target.has_value() || temp_control == Chamber::TemperatureControl {}) {
            return HeatState::stable;

        } else if (*current > *target + tolerance) {
            return HeatState::cooling;

        } else if (*current < *target - tolerance) {
            return HeatState::heating;

        } else {
            return HeatState::stable;
        }
    }();

    return StateAndTemps(state, current.value_or(0), target.value_or(0), false).ToInt();
}

string_view_utf8 FooterItemChamberTemperature::static_makeView(int value) {
    static buffer_t buff;
    return static_makeViewIntoBuff(value, buff);
}
