/**
 * Based on Marlin 3D Printer Firmware
 * Copyright (c) 2022 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 */
#include "../../../inc/MarlinConfig.h"

#include "../../gcode.h"
#include "../../../feature/input_shaper/input_shaper.hpp"
#include "../../../feature/input_shaper/input_shaper_config.hpp"
#include "../../../module/stepper.h"
#include "gcode/parser.h"
#include <config_store/store_instance.hpp>
#include <optional>

namespace input_shaper {

struct M593Params {
    bool seen_x;
    bool seen_y;
    bool seen_z;
    bool seen_w;
    struct {
        std::optional<Type> type;
        std::optional<float> frequency;
        std::optional<float> damping_ratio;
        std::optional<float> vibration_reduction;
    } axis;
    struct {
        std::optional<float> frequency_delta;
        std::optional<float> mass_limit;
    } weight_adjust;
};

static bool contains_axis_change(const M593Params &params) {
    return params.axis.type || params.axis.frequency || params.axis.damping_ratio || params.axis.vibration_reduction;
}

static bool contains_weight_adjust_change(const M593Params &params) {
    return params.weight_adjust.frequency_delta || params.weight_adjust.mass_limit;
}

static bool is_empty(const M593Params &params) {
    return !contains_axis_change(params) && !contains_weight_adjust_change(params) && !params.seen_x && !params.seen_y && !params.seen_z && !params.seen_w;
}

static std::optional<AxisConfig> get_axis_config(const AxisConfig &config, const M593Params &params) {
    if (params.axis.frequency && *params.axis.frequency == 0) {
        return std::nullopt;
    }
    return AxisConfig {
        .type = params.axis.type.value_or(config.type),
        .frequency = params.axis.frequency.value_or(config.frequency),
        .damping_ratio = params.axis.damping_ratio.value_or(config.damping_ratio),
        .vibration_reduction = params.axis.vibration_reduction.value_or(config.vibration_reduction),
    };
}

static std::optional<WeightAdjustConfig> get_weight_adjust_config(const WeightAdjustConfig &config, const M593Params &params) {
    if (params.weight_adjust.mass_limit && *params.weight_adjust.mass_limit == 0) {
        return std::nullopt;
    }
    return WeightAdjustConfig {
        .frequency_delta = params.weight_adjust.frequency_delta.value_or(config.frequency_delta),
        .mass_limit = params.weight_adjust.mass_limit.value_or(config.mass_limit),
    };
}

static void dump_axis_config(const AxisEnum axis, const AxisConfig &c) {
    char buff[128];
    snprintf(buff, 128, "axis %c type=%s freq=%f damp=%f vr=%f", axis_codes[axis], to_string(c.type), c.frequency, c.damping_ratio, c.vibration_reduction);
    SERIAL_ECHO_START();
    SERIAL_ECHOLN(buff);
}

static void dump_weight_adjust_config(const char *prefix, const WeightAdjustConfig &c) {
    char buff[128];
    snprintf(buff, 128, "%s freq_delta=%f mass_limit=%f", prefix, c.frequency_delta, c.mass_limit);
    SERIAL_ECHO_START();
    SERIAL_ECHOLN(buff);
}

static void dump_current_config() {
    LOOP_XYZ(i) {
        if (const auto &axis_config = current_config().axis[i]) {
            dump_axis_config((AxisEnum)i, *axis_config);
        } else {
            SERIAL_ECHO_START();
            SERIAL_ECHOLNPAIR("axis ", axis_codes[i], " disabled");
        }
    }
    if (const auto &weight_adjust_y = current_config().weight_adjust_y) {
        dump_weight_adjust_config("weight_adjust y", *weight_adjust_y);
    } else {
        SERIAL_ECHO_MSG("weight adjust y disabled");
    }
}

static M593Params clamp_frequency(M593Params params) {
    if (params.axis.frequency != 0.) {
        const float original_frequency = *params.axis.frequency;
        const float clamped_frequency = clamp_frequency_to_safe_values(original_frequency);
        if (clamped_frequency != original_frequency) {
            SERIAL_ECHO_MSG("Frequency clamped to safe values");
            params.axis.frequency = clamped_frequency;
        }
    }
    return params;
}

static void M593_set_axis_config(const AxisEnum axis, const M593Params &params) {
    const AxisConfig &prev_config = current_config().axis[axis].value_or(axis_defaults[axis]);
    const std::optional<AxisConfig> next_config = get_axis_config(prev_config, clamp_frequency(params));
    set_axis_config(axis, next_config);
    set_config_for_m74(axis, next_config);
}

static void M593_internal(const M593Params &params) {
    if (contains_axis_change(params)) {
        if (params.seen_x || (!params.seen_y && !params.seen_z)) {
            M593_set_axis_config(X_AXIS, params);
        }
        if (params.seen_y || (!params.seen_x && !params.seen_z)) {
            M593_set_axis_config(Y_AXIS, params);
        }
        if (params.seen_z || (!params.seen_x && !params.seen_y)) {
            M593_set_axis_config(Z_AXIS, params);
        }
    }
    if (contains_weight_adjust_change(params)) {
        const WeightAdjustConfig prev_config = current_config().weight_adjust_y.value_or(weight_adjust_y_default);
        const std::optional<WeightAdjustConfig> next_config = get_weight_adjust_config(prev_config, params);
        current_config().weight_adjust_y = next_config;
        set_config_for_m74(next_config);
    }
    if (params.seen_w) {
        config_store().set_input_shaper_config(current_config());
    }
    if (is_empty(params)) {
        dump_current_config();
    }
}

} // namespace input_shaper

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M593: Configure Input Shaping  <a href="https://reprap.org/wiki/G-code#M593:_Configure_Input_Shaping">M593: Configure Input Shaping</a>
 *
 *#### Usage
 *
 *    M593 [ D | F | T | R | X | Y | Z | A | M | W ]
 *
 *#### Parameters
 *
 * - `D` - Set damping ratio. Range 0 to 1.
 * - `F` - Set frequency. Greater or equal to 0.
 *   - `0` - Default value is 0Hz = input shaper is disabled.
 * - `T` - Set type. Range 0 to 5
 *   - `0` - ZV Default
 *   - `1` - ZVD
 *   - `2` - MZV
 *   - `3` - EI
 *   - `4` - 2HUMP_EI
 *   - `5` - 3HUMP_EI
 * - `R` - Set vibration reduction. Greater than 0. Default value is 20.
 * - `X` - X axis
 * - `Y` - Y axis
 * - `Z` - Z axis
 * - `A` - Weight adjust frequency delta.
 * - `M` - Weight adjust mass limit.
 * - `W` - Write current input shaper settings to EEPROM.
 *
 * Without parameters prints the current Input Shaping settings
 */
void GcodeSuite::M593() {
    input_shaper::M593Params params;
    params.seen_x = parser.seen('X');
    params.seen_y = parser.seen('Y');
    params.seen_z = parser.seen('Z');
    params.seen_w = parser.seen('W');

    if (parser.seen('D')) {
        const float dr = parser.value_float();
        if (WITHIN(dr, 0., 1.)) {
            params.axis.damping_ratio = dr;
        } else {
            SERIAL_ECHO_MSG("?Damping ratio (D) value out of range (0-1)");
        }
    }

    if (parser.seen('F')) {
        const float f = parser.value_float();
        if (f >= 0) {
            params.axis.frequency = f;
        } else {
            SERIAL_ECHO_MSG("?Frequency (X) must be greater or equal to 0");
        }
    }

    if (parser.seen('T')) {
        const int t = parser.value_int();
        if (WITHIN(t, static_cast<int>(input_shaper::Type::first), static_cast<int>(input_shaper::Type::last))) {
            params.axis.type = static_cast<input_shaper::Type>(t);
        } else {
            SERIAL_ECHO_MSG("?Invalid type of input shaper (T)");
        }
    }

    if (parser.seen('R')) {
        const float vr = parser.value_float();
        if (vr > 0) {
            params.axis.vibration_reduction = vr;
        } else {
            SERIAL_ECHO_MSG("?Vibration reduction (X) must be greater than 0");
        }
    }

    if (parser.seen('A')) {
        const float a = parser.value_float();
        if (a < 0) {
            params.weight_adjust.frequency_delta = a;
        } else {
            SERIAL_ECHO_MSG("?Weight adjust frequency delta (A) must be lesser than 0");
        }
    }

    if (parser.seen('M')) {
        const float m = parser.value_float();
        if (m >= 0) {
            params.weight_adjust.mass_limit = m;
        } else {
            SERIAL_ECHO_MSG("?Weight adjust mass limit (M) must be greater or equal to 0");
        }
    }

    input_shaper::M593_internal(params);
}

/** @}*/
