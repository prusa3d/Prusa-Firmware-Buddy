#include "../../gcode.h"
#include "feature/input_shaper/input_shaper.hpp"
#include "feature/input_shaper/input_shaper_config.hpp"
#include <cmath>

namespace input_shaper {

struct M74Params {
    std::optional<float> mass;
};

float get_weight_adjusted_frequency(float mass, const AxisConfig &axis, const WeightAdjustConfig &weight_adjust) {
    const float axis_frequency = axis.frequency;
    const float mass_limit = weight_adjust.mass_limit;
    const float frequency_limit = axis.frequency + weight_adjust.frequency_delta;
    if (mass > mass_limit) {
        return frequency_limit;
    }
    if (mass_limit == 0) {
        return axis_frequency;
    }
    const float t = mass / mass_limit;
    return std::lerp(axis_frequency, frequency_limit, t);
}

void M74_internal(const M74Params &params) {
    const auto &config = current_config();
    if (params.mass && config.axis_y && config.weight_adjust_y) {
        float frequency = get_weight_adjusted_frequency(*params.mass, *config.axis_y, *config.weight_adjust_y);
        set_axis_y_config(AxisConfig {
            .type = config.axis_y->type,
            .frequency = frequency,
            .damping_ratio = config.axis_y->damping_ratio,
            .vibration_reduction = config.axis_y->vibration_reduction,
        });
    }
}

} // namespace input_shaper

/**
 * @brief Set mass.
 *
 * - W<mass>      Set the total mass in grams of everything that is currently sitting on the bed.
 */
void GcodeSuite::M74() {
    input_shaper::M74Params params;

    if (parser.seen('W')) {
        const float m = parser.value_float();
        if (m > 0) {
            params.mass = m;
        } else {
            SERIAL_ECHO_MSG("?Mass (W) must be greater than 0");
        }
    }

    input_shaper::M74_internal(params);
}
