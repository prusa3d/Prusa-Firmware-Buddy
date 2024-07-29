#pragma once

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <variant>

// !!! DO NOT CHANGE - this is used in config store
/// Maximum length of a filament name, including the terminating zero
constexpr size_t filament_name_buffer_size = 8;

/// Maximum ever expected preset filament type count
constexpr size_t max_preset_filament_type_count = 32;

/// Maximum ever expected user filament type count
constexpr size_t max_user_filament_type_count = 32;

/// Maximum ever expected count of  all filament types
constexpr size_t max_total_filament_count = max_user_filament_type_count + max_preset_filament_type_count;

/// Actually defined user filament type count
constexpr size_t user_filament_type_count = 8;

/// Should match extruder count (or be higher), one for each extruder
/// Hardcoded to prevent dependency pollution
constexpr size_t adhoc_filament_type_count = 6;

// !!! DO NOT CHANGE - this is used in config store
struct __attribute__((packed)) FilamentTypeParameters {

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

    /// Whether the filament is abrasive and requires hardened (abrasive-resistant) nozzle
    bool is_abrasive : 1 = false;

    // Keeping the remaining bits of the bitfield unused, but zero initizliazed, for future proofing
    uint8_t _unused : 6 = 0;

public:
    constexpr bool operator==(const FilamentTypeParameters &) const = default;
    constexpr bool operator!=(const FilamentTypeParameters &) const = default;
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

static constexpr size_t preset_filament_type_count = static_cast<size_t>(PresetFilamentType::_count);

/// User-configurable "presets" for filaments
struct UserFilamentType {
    uint8_t index = 0;

    inline constexpr bool operator==(const UserFilamentType &) const = default;
    inline constexpr bool operator!=(const UserFilamentType &) const = default;
};

/// Ad-hoc filament type, adjustable, one for each toolhead.
/// Not listed in all_filament_types.
/// The parameters can be entered directly during preheat by selecting a special option in the preheat menu.
struct AdHocFilamentType {
    uint8_t tool = 0;

    inline constexpr bool operator==(const AdHocFilamentType &) const = default;
    inline constexpr bool operator!=(const AdHocFilamentType &) const = default;
};

struct NoFilamentType {
    inline constexpr bool operator==(const NoFilamentType &) const = default;
    inline constexpr bool operator!=(const NoFilamentType &) const = default;
};

using FilamentType_ = std::variant<NoFilamentType, PresetFilamentType, UserFilamentType, AdHocFilamentType>;

/// Count of all filament types
constexpr size_t total_filament_type_count = preset_filament_type_count + user_filament_type_count;

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

    /// \returns filament type with the corresponding name
    /// !!! This disregards ad-hoc filament types
    static FilamentType from_name(const std::string_view &name);

    /// \returns whether the filament type is of the specified name.
    /// !!! Prefer using "loaded_filament.matches(b_name)" over "loaded_filament == FilamentType::from_name(b_name)" where it makes sense.
    /// !!! This is because "loaded_filament" could be an ad-hoc filament, which is never returned from FilamentType::from_name.
    bool matches(const std::string_view &name) const;

    /// \returns parameters of the filament type
    FilamentTypeParameters parameters() const;

    /// Sets parameters of the filament type, provided that the filament type is customizable
    void set_parameters(const FilamentTypeParameters &set) const;

    /// \returns Modified parameters of the filament type (calls \p f(params) and stores the modified data)
    template <class F>
    void modify_parameters(F &&f) const {
        FilamentTypeParameters params = parameters();
        f(params);
        set_parameters(params);
    }

    /// \returns whether the filaments parameters can be adjusted by the user
    inline bool is_customizable() const {
        return std::holds_alternative<UserFilamentType>(*this) || std::holds_alternative<AdHocFilamentType>(*this);
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
