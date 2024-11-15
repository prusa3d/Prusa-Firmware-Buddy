#pragma once

#include <cstdint>
#include <optional>
#include <printers.h>
#include <utility_extensions.hpp>
#include "enum_array.hpp"

enum AxisEnum : uint8_t; // FWD declaration to avoid Marlin dependency in tests

namespace input_shaper {

enum class Type : uint8_t {
    // DO NOT CHANGE VALUES IN THIS ENUM WITHOUT CHANGING EEPROM CODE!
    // We no longer recommend ZV, EI_2HUMP & EI_3HUMP filter types with our automatic fitting

    first = 0,
    zv = first,
    zvd,
    mzv,
    ei,
    ei_2hump,
    ei_3hump,
    null,
    last = null,
    cnt
};

static constexpr EnumArray<Type, const char *, ftrstd::to_underlying(Type::cnt)> filter_names {
    { Type::zv, "ZV" },
    { Type::zvd, "ZVD" },
    { Type::mzv, "MZV" },
    { Type::ei, "EI" },
    { Type::ei_2hump, "EI_2HUMP" },
    { Type::ei_3hump, "EI_3HUMP" },
    { Type::null, "Null" }

};

static constexpr EnumArray<Type, const char *, ftrstd::to_underlying(Type::cnt)> filter_short_names {
    { Type::zv, "ZV" },
    { Type::zvd, "ZVD" },
    { Type::mzv, "MZV" },
    { Type::ei, "EI" },
    { Type::ei_2hump, "EI2" },
    { Type::ei_3hump, "EI3" },
    { Type::null, "NUL" }
};

/// Ordered and filtered filter list, for UI purposes
static constexpr std::array filter_list {
    Type::zvd,
    Type::mzv,
    Type::ei,
    Type::null,
};

static constexpr uint8_t low_freq_limit_hz = 35;
static constexpr uint8_t high_freq_limit_hz = 70;

const char *to_string(Type type);
const char *to_short_string(Type type);

static inline constexpr Type operator+(const Type lhs, const std::underlying_type<Type>::type rhs) {
    auto l_value = static_cast<std::underlying_type<Type>::type>(lhs);
    return Type(l_value + rhs);
}

inline Type &operator++(Type &type) {
    type = type + 1;
    return type;
}

struct __attribute__((packed)) AxisConfig {
    // DO NOT CHANGE LAYOUT OF THIS CLASS WITHOUT CHANGING EEPROM CODE!

    Type type;
    char _padding[3] {};
    float frequency; // Hz
    float damping_ratio;
    float vibration_reduction;

    friend auto operator<=>(const AxisConfig &, const AxisConfig &) = default;
};

struct __attribute__((packed)) WeightAdjustConfig {
    // DO NOT CHANGE LAYOUT OF THIS CLASS WITHOUT CHANGING EEPROM CODE!

    float frequency_delta; // Hz
    float mass_limit; // g

    friend auto operator<=>(const WeightAdjustConfig &, const WeightAdjustConfig &) = default;
};

struct Config {
    std::optional<AxisConfig> axis[3];
    std::optional<WeightAdjustConfig> weight_adjust_y;
};

// NOTE: On CoreXY the following apply to the logical movement axis, not the AB directions
inline constexpr AxisConfig axis_x_default {
    // DO NOT CHANGE DEFAULTS WITHOUT CHANGING EEPROM CODE!

#if PRINTER_IS_PRUSA_MINI()
    .type = Type::mzv,
    .frequency = 118.2,
#elif PRINTER_IS_PRUSA_XL()
    .type = Type::mzv,
    .frequency = 35.8,
#else
    .type = Type::mzv,
    .frequency = 50.7f,
#endif
    .damping_ratio = 0.1f,
    .vibration_reduction = 20.0f,
};
inline constexpr AxisConfig axis_y_default {
    // DO NOT CHANGE DEFAULTS WITHOUT CHANGING EEPROM CODE!

#if PRINTER_IS_PRUSA_MINI()
    .type = Type::mzv,
    .frequency = 32.8,
#elif PRINTER_IS_PRUSA_XL()
    .type = Type::mzv,
    .frequency = 35.4,
#else
    .type = Type::mzv,
    .frequency = 40.6f,
#endif
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

inline constexpr AxisConfig axis_defaults[3] = { axis_x_default, axis_y_default, axis_z_default };

inline constexpr bool weight_adjust_enabled_default = {
// DO NOT CHANGE DEFAULTS WITHOUT CHANGING EEPROM CODE!
#if PRINTER_IS_PRUSA_XL() || PRINTER_IS_PRUSA_iX()
    false
#else
    true
#endif
};

inline constexpr WeightAdjustConfig weight_adjust_y_default {
    // DO NOT CHANGE DEFAULTS WITHOUT CHANGING EEPROM CODE!
#if PRINTER_IS_PRUSA_XL() || PRINTER_IS_PRUSA_iX()
    .frequency_delta = 0,
#else
    .frequency_delta = -20.0f,
#endif
    .mass_limit = 800.0f,
};

Config &current_config();

void set_config_for_m74(const AxisEnum axis, const std::optional<AxisConfig> &axis_config);
void set_config_for_m74(const std::optional<WeightAdjustConfig> &next_config);
Config get_config_for_m74();

void init();
void set_axis_config(const AxisEnum axis, std::optional<AxisConfig> axis_config);
void set_axis_y_weight_adjust(std::optional<WeightAdjustConfig> wa_config);

constexpr float frequency_safe_min = 10.0;
#if PRINTER_IS_PRUSA_MINI()
constexpr float frequency_safe_max = 150.0;
#else
constexpr float frequency_safe_max = 100.0;
#endif

float clamp_frequency_to_safe_values(float frequency);

} // namespace input_shaper
