#include "pressure_advance.hpp"
#include "../precise_stepping/precise_stepping.hpp"

namespace pressure_advance {

static Config e_axis_config;

void init() {
    // stub
}

const Config &get_axis_e_config() {
    return e_axis_config;
}

void set_axis_e_config(const Config &config) {
    // ensure moves are not being processed as we change parameters
    assert(PreciseStepping::move_segment_queue_size() == 0);

    // update internal filter parameters
    PressureAdvance::pressure_advance_params = create_pressure_advance_params(config);

    // set step generator
    e_axis_config = config;
    if (config.pressure_advance > 0.f) {
        PreciseStepping::physical_axis_step_generator_types |= PRESSURE_ADVANCE_STEP_GENERATOR_E;
    } else {
        PreciseStepping::physical_axis_step_generator_types &= ~PRESSURE_ADVANCE_STEP_GENERATOR_E;
    }

    PreciseStepping::update_maximum_lookback_time();
}

} // namespace pressure_advance
