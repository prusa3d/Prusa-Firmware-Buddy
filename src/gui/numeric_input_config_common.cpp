#include <numeric_input_config_common.hpp>

#include <Configuration.h>

#if HAS_CHAMBER_API()
    #include <feature/chamber/chamber.hpp>
#endif

const NumericInputConfig numeric_input_config::nozzle_temperature = {
    .max_value = HEATER_0_MAXTEMP - HEATER_MAXTEMP_SAFETY_MARGIN,
    .special_value = 0,
    .unit = Unit::celsius,
};

const NumericInputConfig numeric_input_config::filament_nozzle_temperature = {
    .min_value = EXTRUDE_MINTEMP,
    .max_value = HEATER_0_MAXTEMP - HEATER_MAXTEMP_SAFETY_MARGIN,
    .unit = Unit::celsius,
};

const NumericInputConfig numeric_input_config::bed_temperature = {
    .max_value = (BED_MAXTEMP - BED_MAXTEMP_SAFETY_MARGIN),
    .special_value = 0,
    .unit = Unit::celsius,
};

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
