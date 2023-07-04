#include "input_shaper_config.hpp"
#include "input_shaper.hpp"

#include "eeprom_journal/store_instance.hpp"

#include "../../module/planner.h"

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
    if (!config_for_m74.axis[axis]) {
        config_for_m74.axis[axis] = next_config;
    }
}

void set_config_for_m74(const std::optional<WeightAdjustConfig> &next_config) {
    // This function complements set_config_for_m74 for setting axis config
    if (!config_for_m74.weight_adjust_y) {
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

static input_shaper_pulses_t get_input_shaper(const input_shaper::Type input_shaper_type, const float input_shaper_frequency, const float damping_ratio, const float vibration_reduction) {
    switch (input_shaper_type) {
    case input_shaper::Type::zv:
        return create_zv_input_shaper_pulses(input_shaper_frequency, damping_ratio);
    case input_shaper::Type::zvd:
        return create_zvd_input_shaper_pulses(input_shaper_frequency, damping_ratio);
    case input_shaper::Type::mzv:
        return create_mzv_input_shaper_pulses(input_shaper_frequency, damping_ratio);
    case input_shaper::Type::ei:
        return create_ei_input_shaper_pulses(input_shaper_frequency, damping_ratio, vibration_reduction);
    case input_shaper::Type::ei_2hump:
        return create_2hump_ei_input_shaper_pulses(input_shaper_frequency, damping_ratio, vibration_reduction);
    case input_shaper::Type::ei_3hump:
        return create_3hump_ei_input_shaper_pulses(input_shaper_frequency, damping_ratio, vibration_reduction);
    case input_shaper::Type::null:
        return create_null_input_shaper_pulses();
    default:
        return create_zv_input_shaper_pulses(input_shaper_frequency, damping_ratio);
    }
}

static auto get_input_shaper(const AxisConfig &c) {
    return get_input_shaper(c.type, c.frequency, c.damping_ratio, c.vibration_reduction);
}
static void set_axis_config_internal(const AxisEnum axis, std::optional<AxisConfig> axis_config) {
    if (axis_config) {
        InputShaper::is_pulses[axis] = get_input_shaper(*axis_config);
        PreciseStepping::step_generator_types |= (INPUT_SHAPER_STEP_GENERATOR_X << axis);
    } else {
        PreciseStepping::step_generator_types &= ~(INPUT_SHAPER_STEP_GENERATOR_X << axis);
    }
}

void init() {
    auto &config = current_config();
    config = config_store().get_input_shaper_config();
    LOOP_XYZ(i) {
        set_axis_config_internal((AxisEnum)i, config.axis[i]);
    }
    config_for_m74 = {};
}

void set_axis_config(const AxisEnum axis, std::optional<AxisConfig> axis_config) {
    if (axis_config) {
        axis_config->frequency = clamp_frequency_to_safe_values(axis_config->frequency);
    }
    // For now, we must ensure that all queues are empty before changing input shapers parameters.
    // But later, it could be possible to wait just for block and move quests.
    planner.synchronize();
    set_axis_config_internal(axis, axis_config);
    current_config().axis[axis] = axis_config;
}

void set_axis_y_weight_adjust(std::optional<WeightAdjustConfig> wa_config) {
    // NOTE: this function doesn't recalculate the filter parameters; it just sets the existing
    //   parameters so that further M74 calls produce identical results. Since M74 already
    //   recomputes _current_ Y filter settings, if this assumption gets broken the power_panic
    //   code will require a different recovery procedure.
    current_config().weight_adjust_y = wa_config;
}

const char *to_string(Type type) {
    switch (type) {
    case Type::zv:
        return "ZV";
    case Type::zvd:
        return "ZVD";
    case Type::mzv:
        return "MZV";
    case Type::ei:
        return "EI";
    case Type::ei_2hump:
        return "EI_2HUMP";
    case Type::ei_3hump:
        return "EI_3HUMP";
    case Type::null:
        return "null";
    default:
        break;
    }
    return "Unknown";
}

float clamp_frequency_to_safe_values(float frequency) {
    return std::clamp(frequency, frequency_safe_min, frequency_safe_max);
}

}
