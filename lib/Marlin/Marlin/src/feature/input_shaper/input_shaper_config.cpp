#include "input_shaper_config.hpp"
#include "input_shaper.hpp"

#include "eeprom_journal_config_store.hpp"

#include "../../module/planner.h"

namespace input_shaper {

Config &current_config() {
    static Config config = config_store().get_input_shaper_config();
    return config;
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
static void set_axis_x_config_internal(std::optional<AxisConfig> axis_config) {
    if (axis_config) {
        InputShaper::is_pulses_x = get_input_shaper(*axis_config);
        PreciseStepping::step_generator_types |= INPUT_SHAPER_STEP_GENERATOR_X;
    } else {
        PreciseStepping::step_generator_types &= ~INPUT_SHAPER_STEP_GENERATOR_X;
    }
}

static void set_axis_y_config_internal(std::optional<AxisConfig> axis_config) {
    if (axis_config) {
        InputShaper::is_pulses_y = get_input_shaper(*axis_config);
        PreciseStepping::step_generator_types |= INPUT_SHAPER_STEP_GENERATOR_Y;
    } else {
        PreciseStepping::step_generator_types &= ~INPUT_SHAPER_STEP_GENERATOR_Y;
    }
}

static void set_axis_z_config_internal(std::optional<AxisConfig> axis_config) {
    if (axis_config) {
        InputShaper::is_pulses_z = get_input_shaper(*axis_config);
        PreciseStepping::step_generator_types |= INPUT_SHAPER_STEP_GENERATOR_Z;
    } else {
        PreciseStepping::step_generator_types &= ~INPUT_SHAPER_STEP_GENERATOR_Z;
    }
}

void init() {
    auto &config = current_config();
    config = config_store().get_input_shaper_config();
    set_axis_x_config_internal(config.axis_x);
    set_axis_y_config_internal(config.axis_y);
    set_axis_z_config_internal(config.axis_z);
}

void set_axis_x_config(std::optional<AxisConfig> axis_config) {
    // For now, we must ensure that all queues are empty before changing input shapers parameters.
    // But later, it could be possible to wait just for block and move quests.
    planner.synchronize();
    set_axis_x_config_internal(axis_config);
    current_config().axis_x = axis_config;
}

void set_axis_y_config(std::optional<AxisConfig> axis_config) {
    // For now, we must ensure that all queues are empty before changing input shapers parameters.
    // But later, it could be possible to wait just for block and move quests.
    planner.synchronize();
    set_axis_y_config_internal(axis_config);
    current_config().axis_y = axis_config;
}

void set_axis_z_config(std::optional<AxisConfig> axis_config) {
    // For now, we must ensure that all queues are empty before changing input shapers parameters.
    // But later, it could be possible to wait just for block and move quests.
    planner.synchronize();
    set_axis_z_config_internal(axis_config);
    current_config().axis_z = axis_config;
}

void set_axis_y_weight_adjust(std::optional<WeightAdjustConfig> wa_config) {
    // NOTE: this function doesn't recalculate the filter parameters; it just sets the existing
    //   parameters so that further M74 calls produce identical results. Since M74 already
    //   recomputes _current_ Y filter settings, if this assumption gets broken the power_panic
    //   code will require a different recovery procedure.
    current_config().weight_adjust_y = wa_config;
}

}
