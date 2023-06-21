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
#include "configuration_store.hpp"
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

static const char *to_string(Type type) {
    switch (type) {
    case Type::zv:
        return "zv";
    case Type::zvd:
        return "zvd";
    case Type::mzv:
        return "mzv";
    case Type::ei:
        return "ei";
    case Type::ei_2hump:
        return "ei_2hump";
    case Type::ei_3hump:
        return "ei_3hump";
    case Type::null:
        return "null";
    }
    return "unknown";
}

static void dump_axis_config(const char *prefix, const AxisConfig &c) {
    char buff[128];
    snprintf(buff, 128, "%s type=%s freq=%f damp=%f vr=%f", prefix, to_string(c.type), c.frequency, c.damping_ratio, c.vibration_reduction);
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
    if (const auto &axis_x = current_config().axis_x) {
        dump_axis_config("axis x", *axis_x);
    } else {
        SERIAL_ECHO_MSG("axis x disabled");
    }
    if (const auto &axis_y = current_config().axis_y) {
        dump_axis_config("axis y", *axis_y);
    } else {
        SERIAL_ECHO_MSG("axis y disabled");
    }
    if (const auto &weight_adjust_y = current_config().weight_adjust_y) {
        dump_weight_adjust_config("weight_adjust y", *weight_adjust_y);
    } else {
        SERIAL_ECHO_MSG("weight adjust y disabled");
    }
    if (const auto &axis_z = current_config().axis_z) {
        dump_axis_config("axis z", *axis_z);
    } else {
        SERIAL_ECHO_MSG("axis z disabled");
    }
}

static void M593_internal(const M593Params &params) {
    if (contains_axis_change(params)) {
        if (params.seen_x || (!params.seen_y && !params.seen_z)) {
            const AxisConfig prev_axis = current_config().axis_x.value_or(axis_x_default);
            set_axis_x_config(get_axis_config(prev_axis, params));
        }
        if (params.seen_y || (!params.seen_x && !params.seen_z)) {
            const AxisConfig prev_axis = current_config().axis_y.value_or(axis_y_default);
            set_axis_y_config(get_axis_config(prev_axis, params));
        }
        if (params.seen_z || (!params.seen_x && !params.seen_y)) {
            const AxisConfig prev_axis = current_config().axis_z.value_or(axis_z_default);
            set_axis_z_config(get_axis_config(prev_axis, params));
        }
    }
    if (contains_weight_adjust_change(params)) {
        const WeightAdjustConfig prev = current_config().weight_adjust_y.value_or(weight_adjust_y_default);
        current_config().weight_adjust_y = get_weight_adjust_config(prev, params);
    }
    if (params.seen_w) {
        config_store().set_input_shaper_config(current_config());
    }
    if (is_empty(params)) {
        dump_current_config();
    }
}

} // namespace input_shaper

/**
 * @brief Set parameters for input shapers.
 *
 * - D<ratio>     Set the input shaper damping ratio. If axes (X, Y, etc.) are not specified, set it for all axes. Default value is 0.1.
 * - F<frequency> Set the input shaper frequency. If axes (X, Y, etc.) are not specified, set it for all axes. Default value is 0Hz - It means that the input shaper is disabled.
 * - T[map]       Set the input shaper type, 0:ZV, 1:ZVD, 2:MZV, 3:EI, 4:2HUMP_EI, and 5:3HUMP_EI. Default value is 0:ZV.
 * - R<reduction> Set the input shaper vibration reduction. This parameter is used just for 3:EI, 4:2HUMP_EI, and 5:3HUMP_EI. Default value is 20.
 * - X<1>         Set the input shaper parameters only for the X axis.
 * - Y<1>         Set the input shaper parameters only for the Y axis.
 * - Z<1>         Set the input shaper parameters only for the Z axis.
 * - A<frequency> Set the input shaper weight adjust frequency delta.
 * - M<mass>      Set the input shaper weight adjust mass limit.
 * - W<1>         Write current input shaper settings to EEPROM.
 */
void GcodeSuite::M593() {
    input_shaper::M593Params params;
    params.seen_x = parser.seen('X');
    params.seen_y = parser.seen('Y');
    params.seen_z = parser.seen('Z');
    params.seen_w = parser.seen('W');

    if (parser.seen('D')) {
        const float dr = parser.value_float();
        if (WITHIN(dr, 0., 1.))
            params.axis.damping_ratio = dr;
        else
            SERIAL_ECHO_MSG("?Damping ratio (D) value out of range (0-1)");
    }

    if (parser.seen('F')) {
        const float f = parser.value_float();
        if (f >= 0)
            params.axis.frequency = f;
        else
            SERIAL_ECHO_MSG("?Frequency (X) must be greater or equal to 0");
    }

    if (parser.seen('T')) {
        const int t = parser.value_int();
        if (WITHIN(t, 0, (int)input_shaper::Type::last))
            params.axis.type = static_cast<input_shaper::Type>(t);
        else
            SERIAL_ECHO_MSG("?Invalid type of input shaper (T)");
    }

    if (parser.seen('R')) {
        const float vr = parser.value_float();
        if (vr > 0)
            params.axis.vibration_reduction = vr;
        else
            SERIAL_ECHO_MSG("?Vibration reduction (X) must be greater than 0");
    }

    if (parser.seen('A')) {
        const float a = parser.value_float();
        if (a < 0)
            params.weight_adjust.frequency_delta = a;
        else
            SERIAL_ECHO_MSG("?Weight adjust frequency delta (A) must be lesser than 0");
    }

    if (parser.seen('M')) {
        const float m = parser.value_float();
        if (m > 0)
            params.weight_adjust.mass_limit = m;
        else
            SERIAL_ECHO_MSG("?Weight adjust mass limit (M) must be greater than 0");
    }

    input_shaper::M593_internal(params);
}
