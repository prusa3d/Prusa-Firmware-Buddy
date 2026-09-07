#include <numeric_input_config_common.hpp>

#include <algorithm>

#include <Configuration.h>
#include <tool/hotend/hotend.hpp>
#include <tool/physical_tool.hpp>
#include <utils/overloaded_visitor.hpp>

#if HAS_CHAMBER_API()
    #include <feature/chamber/chamber.hpp>
#endif

static float max_nozzle_target_temp(std::variant<PhysicalToolIndex, AllTools> tool) {
    const Hotend::TargetTemperature max_nozzle_temp = match(
        tool,
        [](PhysicalToolIndex t) { return Hotend::for_tool(t).max_nozzle_temp(); },
        [](AllTools) {
            // A filament preset is not tied to a tool, so offer the range of the hottest installed hotend.
            Hotend::TargetTemperature m = 0;
            for (PhysicalToolIndex t : PhysicalToolIndex::all().skip_all_disabled()) {
                m = std::max(m, Hotend::for_tool(t).max_nozzle_temp());
            }
            return m;
        });
    return max_nozzle_temp - HEATER_MAXTEMP_SAFETY_MARGIN;
}

NumericInputConfig numeric_input_config::nozzle_temperature(std::variant<PhysicalToolIndex, AllTools> tool) {
    return {
        .max_value = max_nozzle_target_temp(tool),
        .special_value = 0,
        .unit = Unit::celsius,
    };
}

NumericInputConfig numeric_input_config::filament_nozzle_temperature(std::variant<PhysicalToolIndex, AllTools> tool) {
    return {
        .min_value = EXTRUDE_MINTEMP,
        .max_value = max_nozzle_target_temp(tool),
        .unit = Unit::celsius,
    };
}

const NumericInputConfig numeric_input_config::bed_temperature = {
    .max_value = (BED_MAXTEMP - BED_MAXTEMP_SAFETY_MARGIN),
    .special_value = 0,
    .unit = Unit::celsius,
};

#if HAS_HEATBREAK_TEMP()
const NumericInputConfig numeric_input_config::heatbreak_temperature = {
    .min_value = HEATBREAK_MINTEMP,
    .max_value = HEATBREAK_MAXTEMP,
    .unit = Unit::celsius,
};
#endif

const NumericInputConfig numeric_input_config::percent_with_off = {
    .max_value = 100,
    .special_value = 0,
    .unit = Unit::percent,
};

const NumericInputConfig numeric_input_config::percent_with_auto = {
    .max_value = 100,
    .special_value = -1,
    .special_value_str = N_("Auto"),
    .unit = Unit::percent,
};

const NumericInputConfig numeric_input_config::percent_with_disabled = {
    .max_value = 100,
    .special_value = -1,
    .special_value_str = N_("Disabled"),
    .unit = Unit::percent,
};

#if HAS_CHAMBER_API()
const NumericInputConfig &numeric_input_config::chamber_temp_with_off() {
    // Using static is intended here and okay - we need to be returing a persistent reference, and this function is only used from the GUI thread
    static NumericInputConfig config;
    config = {
        .max_value = buddy::chamber().capabilities().max_temp.value_or(100),
        .special_value = 0,
        .unit = Unit::celsius,
    };
    return config;
}

const NumericInputConfig &numeric_input_config::chamber_temp_with_none() {
    // Using static is intended here and okay - we need to be returing a persistent reference, and this function is only used from the GUI thread
    static NumericInputConfig config;
    config = chamber_temp_with_off();
    config.special_value_str = N_("None");
    return config;
}
#endif
