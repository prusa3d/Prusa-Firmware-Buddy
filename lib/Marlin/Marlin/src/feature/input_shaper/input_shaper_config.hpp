#pragma once

#include <cstdint>
#include <optional>

namespace input_shaper {

enum class Type : uint8_t {
    // DO NOT CHANGE VALUES IN THIS ENUM WITHOUT CHANGING EEPROM CODE!

    first = 0,
    zv = first,
    second,
    zvd = second,
    mzv,
    ei,
    ei_2hump,
    ei_3hump,
    null,
    last = null
};

inline Type &operator++(Type &type) {
    using IntType = typename std::underlying_type<Type>::type;
    type = static_cast<Type>(static_cast<IntType>(type) + 1);
    return type;
}

struct __attribute__((packed)) AxisConfig {
    // DO NOT CHANGE LAYOUT OF THIS CLASS WITHOUT CHANGING EEPROM CODE!

    Type type;
    char _padding[3];
    float frequency; // Hz
    float damping_ratio;
    float vibration_reduction;

    friend auto operator<=>(const AxisConfig &, const AxisConfig &) = default;
};

struct __attribute__((packed)) WeightAdjustConfig {
    // DO NOT CHANGE LAYOUT OF THIS CLASS WITHOUT CHANGING EEPROM CODE!

    float frequency_delta; // Hz
    float mass_limit;      // g

    friend auto operator<=>(const WeightAdjustConfig &, const WeightAdjustConfig &) = default;
};

struct Config {
    std::optional<AxisConfig> axis_x;
    std::optional<AxisConfig> axis_y;
    std::optional<WeightAdjustConfig> weight_adjust_y;
    std::optional<AxisConfig> axis_z;
};

inline constexpr AxisConfig axis_x_default {
    // DO NOT CHANGE DEFAULTS WITHOUT CHANGING EEPROM CODE!

    .type = Type::mzv,
    .frequency = 50.7f,
    .damping_ratio = 0.1f,
    .vibration_reduction = 20.0f,
};
inline constexpr AxisConfig axis_y_default {
    // DO NOT CHANGE DEFAULTS WITHOUT CHANGING EEPROM CODE!

    .type = Type::mzv,
    .frequency = 40.6f,
    .damping_ratio = 0.1f,
    .vibration_reduction = 20.0f,
};
inline constexpr AxisConfig axis_z_default {
    // Z axis does not default to IS and is not currently saved on eeprom.
    // Filter type/frequency is only set at runtime and can be freely changed.

    .type = Type::null,
    .frequency = 0.,
    .damping_ratio = 0.,
    .vibration_reduction = 0.,
};

inline constexpr WeightAdjustConfig weight_adjust_y_default {
    // DO NOT CHANGE DEFAULTS WITHOUT CHANGING EEPROM CODE!

    .frequency_delta = -20.0f,
    .mass_limit = 800.0f,
};

Config &current_config();

void init();
void set_axis_x_config(std::optional<AxisConfig> axis_config);
void set_axis_y_config(std::optional<AxisConfig> axis_config);
void set_axis_y_weight_adjust(std::optional<WeightAdjustConfig> wa_config);
void set_axis_z_config(std::optional<AxisConfig> axis_config);

}
