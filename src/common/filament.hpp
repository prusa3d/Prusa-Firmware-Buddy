#pragma once

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <variant>

// !!! DO NOT CHANGE - this is used in config store
/// Maximum length of a filament name, including the terminating zero
constexpr size_t filament_name_buffer_size = 8;

/// Maximum ever expected filament types.
constexpr size_t max_preset_filament_type_count = 32;

struct FilamentTypeParameters {

public:
    /// Name of the filament (zero terminated).
    /// Keeping this as not array for ease of assignment and reading using snprintf
    char name[filament_name_buffer_size] = "";

    /// Nozzle temperature for the filament, in degrees Celsius
    uint16_t nozzle_temperature;

    /// Nozzle preheat temperature for the filament, in degrees Celsius
    uint16_t nozzle_preheat_temperature = 170;

    /// Bed temperature for the filament, in degrees Celsius
    uint8_t heatbed_temperature;

    /// Whether the filament requires filtration (used in XL enclosure)
    bool requires_filtration : 1 = false;

    // Keeping the remaining bits of the bitfield unused, but zero initizliazed, for future proofing
    uint8_t _unused : 7 = 0;
};

// !!! DO NOT REORDER, DO NOT CHANGE - this is used in config store
enum class PresetFilamentType : uint8_t {
    PLA = 0,
    PETG = 1,
    ASA = 2,
    PC = 3,
    PVB = 4,
    ABS = 5,
    HIPS = 6,
    PP = 7,
    FLEX = 8,
    PA = 9,

    _count
};

struct NoFilamentType {
    inline constexpr bool operator==(const NoFilamentType &) const = default;
    inline constexpr bool operator!=(const NoFilamentType &) const = default;
};

using FilamentType_ = std::variant<NoFilamentType, PresetFilamentType>;

/// Count of all filament types
constexpr size_t total_filament_type_count = static_cast<size_t>(PresetFilamentType::_count);

// TODO: Add variant option for user filament types
struct FilamentType : public FilamentType_ {

public:
    // For FilamentType::none
    static constexpr NoFilamentType none = {};

public:
    // * Constructors

    // Inherit parent constructors
    using FilamentType_::FilamentType_;

    constexpr FilamentType()
        : variant(NoFilamentType {}) {}

public:
    // * Name/parameters

    static FilamentType from_name(std::string_view name);

    static inline FilamentType from_name_array(const std::array<char, filament_name_buffer_size> &name) {
        return from_name(std::string_view(name.data()));
    }

    /// \returns parameters of the filament type
    FilamentTypeParameters parameters() const;

    /// \returns whether the filaments parameters can be adjusted by the user
    inline bool is_customizable() const {
        // TODO: user defined filaments
        return false;
    }

    /// \returns whether the filament is visible - shown in standard filament lists
    bool is_visible() const;

    void set_visible(bool set) const;

public:
    // * Operators

    explicit operator bool() const {
        return !std::holds_alternative<NoFilamentType>(*this);
    }

    inline constexpr bool operator==(const FilamentType &) const = default;
    inline constexpr bool operator!=(const FilamentType &) const = default;
};
