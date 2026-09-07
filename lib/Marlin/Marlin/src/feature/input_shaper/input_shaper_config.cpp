#include "input_shaper_config.hpp"
#include "input_shaper.hpp"
#include <utility_extensions.hpp>
#include <config_store/store_instance.hpp>

#include "../../module/planner.h"
#include <bsod/bsod.h>

namespace input_shaper {

Config &current_config() {
    static Config config = config_store().get_input_shaper_config();
    return config;
}

static Config config_for_m74;

void set_config_for_m74(const AxisEnum axis, const std::optional<AxisConfig> &next_config) {
    // Only set the value if it was not set before.
    // Older slicer versions issued both M593 and M74 which caused M74 to adjust already adjusted value.
    // This gets reset after the boot and after the print is done.
    if (!next_config || !config_for_m74.axis[axis]) {
        config_for_m74.axis[axis] = next_config;
    }
}

void set_config_for_m74(const std::optional<WeightAdjustConfig> &next_config) {
    // This function complements set_config_for_m74 for setting axis config
    if (!next_config || !config_for_m74.weight_adjust_y) {
        config_for_m74.weight_adjust_y = next_config;
    }
}

Config get_config_for_m74() {
    // M593 might not be issued at all. In that case, we should use value from EEPROM.
    auto result = config_for_m74;
    const Config &eeprom_config = config_store().get_input_shaper_config();
    LOOP_XYZ(i) {
        if (!result.axis[i]) {
            result.axis[i] = eeprom_config.axis[i];
        }
    }
    if (!result.weight_adjust_y) {
        result.weight_adjust_y = eeprom_config.weight_adjust_y;
    }
    return result;
}

static void get_input_shaper(input_shaper_pulses_t &result, const input_shaper::Type input_shaper_type, const float input_shaper_frequency, const float damping_ratio, const float vibration_reduction) {
    switch (input_shaper_type) {
    case input_shaper::Type::zv:
        return create_zv_input_shaper_pulses(result, input_shaper_frequency, damping_ratio);
    case input_shaper::Type::zvd:
        return create_zvd_input_shaper_pulses(result, input_shaper_frequency, damping_ratio);
    case input_shaper::Type::mzv:
        return create_mzv_input_shaper_pulses(result, input_shaper_frequency, damping_ratio);
    case input_shaper::Type::ei:
        return create_ei_input_shaper_pulses(result, input_shaper_frequency, damping_ratio, vibration_reduction);
    case input_shaper::Type::ei_2hump:
        return create_2hump_ei_input_shaper_pulses(result, input_shaper_frequency, damping_ratio, vibration_reduction);
    case input_shaper::Type::ei_3hump:
        return create_3hump_ei_input_shaper_pulses(result, input_shaper_frequency, damping_ratio, vibration_reduction);
    case input_shaper::Type::null:
        return create_null_input_shaper_pulses(result);
    default:
        return create_zv_input_shaper_pulses(result, input_shaper_frequency, damping_ratio);
    }
}

static void get_input_shaper(input_shaper_pulses_t &result, const AxisConfig &c) {
    get_input_shaper(result, c.type, c.frequency, c.damping_ratio, c.vibration_reduction);
}

#ifdef COREXY
// Returns in the referrenced parameters two new input_shaper_pulses_t that both have the same number of pulses and also,
// they both have pulses at the same time (some of them could have zero amplitude).
// When there is some pulse in a specific time that is present in just one input input_shaper_pulses_t,
// then we use zero as amplitude for the second in that specific time.
// This preprocessing doesn't affect the produced micro move segment by the input shaper implementation
// but simplifies the implementation by a lot for CoreXY. Because we can assume that both logical axes
// are always updated at the same time.
static void adjust_input_shaper_pulses_to_match_time_pulses(input_shaper_pulses_t &first_pulses, input_shaper_pulses_t &second_pulses) {
    input_shaper_pulses_t first_pulses_adjust;
    input_shaper_pulses_t second_pulses_adjust;

    if (first_pulses.num_pulses + second_pulses.num_pulses > INPUT_SHAPER_MAX_PULSES) {
        // Number of input shaper pulses exceeds the capacity of pulses buffer
        bsod("pulses buffer too small");
    }

    first_pulses_adjust.num_pulses = 0;
    second_pulses_adjust.num_pulses = 0;
    bool have_prev_pulse = false;
    TimeTicks prev_pulse_t;
    for (int common_idx = 0, first_idx = 0, second_idx = 0; common_idx < (first_pulses.num_pulses + second_pulses.num_pulses); ++common_idx) {
        const bool is_nearest_pulse_from_first = (first_idx != first_pulses.num_pulses) && (second_idx == second_pulses.num_pulses || first_pulses.pulses[first_idx].t <= second_pulses.pulses[second_idx].t);
        const pulse_t &nearest_pulse = is_nearest_pulse_from_first ? first_pulses.pulses[first_idx++] : second_pulses.pulses[second_idx++];

        // Merge if within threshold. Skip on first pulse (no prev to compare against).
        if (have_prev_pulse && nearest_pulse.t - prev_pulse_t <= INPUT_SHAPER_PULSES_MIN_TIME_DIFF) {
            if (is_nearest_pulse_from_first) {
                debug_assert(first_pulses_adjust.pulses[first_pulses_adjust.num_pulses - 1].a == 0.f);
                first_pulses_adjust.pulses[first_pulses_adjust.num_pulses - 1].a += nearest_pulse.a;
            } else {
                debug_assert(second_pulses_adjust.pulses[second_pulses_adjust.num_pulses - 1].a == 0.f);
                second_pulses_adjust.pulses[second_pulses_adjust.num_pulses - 1].a += nearest_pulse.a;
            }
        } else {
            first_pulses_adjust.pulses[first_pulses_adjust.num_pulses++] = nearest_pulse;
            second_pulses_adjust.pulses[second_pulses_adjust.num_pulses++] = nearest_pulse;

            if (is_nearest_pulse_from_first) {
                second_pulses_adjust.pulses[second_pulses_adjust.num_pulses - 1].a = 0.f;
            } else {
                first_pulses_adjust.pulses[first_pulses_adjust.num_pulses - 1].a = 0.f;
            }

            prev_pulse_t = nearest_pulse.t;
            have_prev_pulse = true;
        }
    }

    first_pulses = first_pulses_adjust;
    second_pulses = second_pulses_adjust;
}

void get_input_shaper(input_shaper_pulses_t &first_axis_pulses, const AxisConfig &first_axis_config, input_shaper_pulses_t &second_axis_pulses, const std::optional<AxisConfig> &second_axis_config) {
    get_input_shaper(first_axis_pulses, first_axis_config);
    if (second_axis_config) {
        get_input_shaper(second_axis_pulses, *second_axis_config);
    } else {
        create_null_input_shaper_pulses(second_axis_pulses);
    }
    adjust_input_shaper_pulses_to_match_time_pulses(first_axis_pulses, second_axis_pulses);
}
#endif

static void set_logical_axis_config_internal(const AxisEnum axis, std::optional<AxisConfig> axis_config) {
    // ensure moves are not being processed as we change parameters
    debug_assert(PreciseStepping::move_segment_queue_size() == 0);

#ifdef COREXY
    const AxisEnum second_axis = (axis == X_AXIS) ? Y_AXIS : X_AXIS;
    if (axis_config) {
        if (axis == X_AXIS || axis == Y_AXIS) {
            std::optional<AxisConfig> second_axis_config = current_config().axis[second_axis];
            get_input_shaper(InputShaper::logical_axis_pulses[axis], *axis_config, InputShaper::logical_axis_pulses[second_axis], second_axis_config);
            PreciseStepping::physical_axis_step_generator_types |= INPUT_SHAPER_STEP_GENERATOR_X;
            PreciseStepping::physical_axis_step_generator_types |= INPUT_SHAPER_STEP_GENERATOR_Y;
        } else {
            get_input_shaper(InputShaper::logical_axis_pulses[axis], *axis_config);
            PreciseStepping::physical_axis_step_generator_types |= (INPUT_SHAPER_STEP_GENERATOR_X << axis);
        }
    } else {
        if (axis == X_AXIS || axis == Y_AXIS) {
            if (std::optional<AxisConfig> second_axis_config = current_config().axis[second_axis]; second_axis_config) {
                // The input shaper is also enabled on the second logical axis, so we disable it just on the first axis.
                get_input_shaper(InputShaper::logical_axis_pulses[second_axis], *second_axis_config, InputShaper::logical_axis_pulses[axis], axis_config);
            } else {
                PreciseStepping::physical_axis_step_generator_types &= ~(INPUT_SHAPER_STEP_GENERATOR_X);
                PreciseStepping::physical_axis_step_generator_types &= ~(INPUT_SHAPER_STEP_GENERATOR_Y);
            }
        } else {
            PreciseStepping::physical_axis_step_generator_types &= ~(INPUT_SHAPER_STEP_GENERATOR_X << axis);
        }
    }
#else
    if (axis_config) {
        get_input_shaper(InputShaper::logical_axis_pulses[axis], *axis_config);
        PreciseStepping::physical_axis_step_generator_types |= (INPUT_SHAPER_STEP_GENERATOR_X << axis);
    } else {
        PreciseStepping::physical_axis_step_generator_types &= ~(INPUT_SHAPER_STEP_GENERATOR_X << axis);
    }
#endif

    PreciseStepping::update_maximum_lookback_time();
}

void init() {
    auto &config = current_config();
    config = config_store().get_input_shaper_config();
    LOOP_XYZ(i) {
        set_logical_axis_config_internal((AxisEnum)i, config.axis[i]);
    }
    config_for_m74 = {};
}

std::optional<AxisConfig> get_axis_config(const AxisEnum axis) {
    return current_config().axis[axis];
}

void set_axis_config(const AxisEnum axis, std::optional<AxisConfig> axis_config) {
    if (axis_config) {
        axis_config->frequency = clamp_frequency_to_safe_values(axis_config->frequency);
    }
    // For now, we must ensure that all queues are empty before changing input shapers parameters.
    // But later, it could be possible to wait just for block and move quests.
    planner.synchronize();
    if (!planner.draining()) {
        // Only set configuration when the current command isn't aborted
        set_logical_axis_config_internal(axis, axis_config);
        current_config().axis[axis] = axis_config;
    }
}

void set_axis_y_weight_adjust(std::optional<WeightAdjustConfig> wa_config) {
    // NOTE: this function doesn't recalculate the filter parameters; it just sets the existing
    //   parameters so that further M74 calls produce identical results. Since M74 already
    //   recomputes _current_ Y filter settings, if this assumption gets broken the power_panic
    //   code will require a different recovery procedure.
    current_config().weight_adjust_y = wa_config;
}

const char *to_string(Type type) {
    auto type_idx = std::to_underlying(type);
    if (type_idx > std::to_underlying(Type::last)) {
        return "Unknown";
    }
    return filter_names[type_idx];
}

const char *to_short_string(Type type) {
    auto type_idx = std::to_underlying(type);
    if (type_idx > std::to_underlying(Type::last)) {
        return "UNK";
    }
    return filter_short_names[type_idx];
}

float clamp_frequency_to_safe_values(float frequency) {
    return std::clamp(frequency, frequency_safe_min, frequency_safe_max);
}

} // namespace input_shaper
