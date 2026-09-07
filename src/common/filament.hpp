#pragma once

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <variant>
#include <expected>
#include <array>
#include <string_view>
#include <optional>

#include <utils/string/inplace_string.hpp>
#include <utils/enum_array.hpp>
#include <utils/compact_optional.hpp>
#include <tool_index.hpp>

#include <option/has_chamber_api.h>
#include <option/has_chamber_filtration_api.h>
#include <option/has_filament_heatbreak_param.h>
#include <option/has_filament_base_preset_param.h>
#include <option/has_anfc.h>
#include <option/has_ht_hotend.h>

class StringBuilder;

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

/// Should match virtual tool count (or be higher), one for each virtual tool
constexpr size_t adhoc_filament_type_count = VirtualToolIndex::count;

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
#if HAS_HT_HOTEND()
    PPS = 10,
    PPA = 11,
#endif

    _count
};

// Used in config store, do not change
static_assert(sizeof(PresetFilamentType) == 1);

struct FilamentTypeParameters {

public:
    using Name = InplaceString<filament_name_buffer_size>;
    /// Name of the filament (zero terminated).
    Name name;

    /// Nozzle temperature for the filament, in degrees Celsius
    int16_t nozzle_temperature = 215;

    /// Nozzle preheat temperature for the filament, in degrees Celsius
    int16_t nozzle_preheat_temperature = 170;

    /// Bed temperature for the filament, in degrees Celsius
    int16_t heatbed_temperature = 60;

#if HAS_ANFC()
    /// OpenPrintTag linked to the filament
    /// Note: not using ToolTag::UIDHash here to avoid dependency hell, static_asserted match in the cpp
    uint16_t openprinttag_uid_hash = 0;
#endif

#if HAS_FILAMENT_BASE_PRESET_PARAM()
    using BasePreset = CompactOptional<PresetFilamentType, static_cast<PresetFilamentType>(0xff)>;
    static_assert(PresetFilamentType::_count < BasePreset::nullopt_value);

    // Used in config store, do not change
    static_assert(sizeof(BasePreset) == 1);

    /// Base preset the filament is based on
    /// This can be used to inherit non-standard filament parameters that are not worth exposing to the user,
    /// such as INDX hotend temperature offset parameters
    BasePreset base_preset = std::nullopt;
#endif

#if HAS_FILAMENT_HEATBREAK_PARAM()
    /// Target heatbreak temperature for the filament
    uint8_t heatbreak_temperature = DEFAULT_HEATBREAK_TEMPERATURE;
#endif

#if HAS_CHAMBER_API()
    /// Minimum temperature at which it's recommended to print this material
    std::optional<uint8_t> chamber_min_temperature = std::nullopt;

    /// Maximum temperature at which it's recommended to print this material
    std::optional<uint8_t> chamber_max_temperature = std::nullopt;

    /// Ideal chamber temperature we would like to keep during printing
    std::optional<uint8_t> chamber_target_temperature = std::nullopt;
#endif

#if HAS_CHAMBER_FILTRATION_API()
    /// Whether the filament requires filtration (used in XL enclosure)
    bool requires_filtration = false;
#endif

    /// Whether the filament is abrasive and requires hardened (abrasive-resistant) nozzle
    bool is_abrasive = false;

    /// Whether the filament is flexible - might require special care in some cases
    bool is_flexible = false;

#if HAS_HT_HOTEND()
    /// Whether the filament requires the high-temperature Nextruder idler door.
    /// Preset-only (not persisted to EEPROM for user/adhoc filaments).
    bool requires_ht_idler_door = false;
#endif

public:
    constexpr bool operator==(const FilamentTypeParameters &) const = default;
    constexpr bool operator!=(const FilamentTypeParameters &) const = default;
};

static constexpr size_t preset_filament_type_count = static_cast<size_t>(PresetFilamentType::_count);

extern constinit const EnumArray<PresetFilamentType, FilamentTypeParameters, PresetFilamentType::_count> preset_filament_parameters;

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

/// Ad-hoc filament type that is pending load. Fully adjustable, not listed in all_filament_types.
/// Useful for filament changes, where we want to change loaded filament parameters only after the previous filament is unloaded.
/// In that case, we would configure the pending custom filament type using `M865 X` and the load using `M600 F'#'`
/// Only stored in RAM
struct PendingAdHocFilamentType {
    inline constexpr bool operator==(const PendingAdHocFilamentType &) const = default;
    inline constexpr bool operator!=(const PendingAdHocFilamentType &) const = default;
};

struct NoFilamentType {
    inline constexpr bool operator==(const NoFilamentType &) const = default;
    inline constexpr bool operator!=(const NoFilamentType &) const = default;
};

using FilamentType_ = std::variant<NoFilamentType, PresetFilamentType, UserFilamentType, AdHocFilamentType, PendingAdHocFilamentType>;

/// Count of all filament types
constexpr size_t total_filament_type_count = preset_filament_type_count + user_filament_type_count;

struct FilamentType : public FilamentType_ {

public:
    static const FilamentType none;
    static const FilamentType pending_adhoc;

    static constexpr const char *adhoc_pending_gcode_code = "#";

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

    static std::optional<FilamentType> from_gcode_param(const std::string_view &value);

    /// @return filament loaded to the specified tool according to the config store
    static FilamentType for_tool(std::variant<VirtualToolIndex, NoTool> tool);

    /// Tries really hard to guess a filament that is loaded in the tool.
    /// Can do various heuristics (for example checking GCodeInfo during print)
    /// Does not necessarily have to coincide with the config store.
    /// Returning NoFilament is a last resort.
    static FilamentType for_tool_heuristic(std::variant<VirtualToolIndex, NoTool> tool);

    /// Guesses FilamentType for current tool
    /// @return for_current_tool_heuristic for current tool
    static FilamentType for_current_tool_heuristic();

    /// \returns whether the filament type is of the specified name.
    /// !!! Prefer using "loaded_filament.matches(b_name)" over "loaded_filament == FilamentType::from_name(b_name)" where it makes sense.
    /// !!! This is because "loaded_filament" could be an ad-hoc filament, which is never returned from FilamentType::from_name.
    bool matches(const std::string_view &name) const;

    /// Appends name of the filament to the builder
    /// If the filament type is non-preset, it visually distincts it (for example by appending PLA (Custom))
    /// Pulls the filament name from FilamentTypeParameters
    void build_name_with_info(StringBuilder &builder) const;

    /// Overload that does not fetch from FilamentTypeParameters
    void build_name_with_info(std::string_view filament_name, StringBuilder &builder) const;

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
        // Revise if filament types are changed/Added
        static_assert(std::is_same_v<FilamentType_, std::variant<NoFilamentType, PresetFilamentType, UserFilamentType, AdHocFilamentType, PendingAdHocFilamentType>>);
        return std::holds_alternative<UserFilamentType>(*this) || std::holds_alternative<AdHocFilamentType>(*this) || std::holds_alternative<PendingAdHocFilamentType>(*this);
    }

    /// \returns whether the filament has a visibility parameter
    inline bool is_visibility_customizable() const {
        // Revise if filament types are changed/Added
        static_assert(std::is_same_v<FilamentType_, std::variant<NoFilamentType, PresetFilamentType, UserFilamentType, AdHocFilamentType, PendingAdHocFilamentType>>);
        return !std::holds_alternative<NoFilamentType>(*this) && !std::holds_alternative<AdHocFilamentType>(*this) && !std::holds_alternative<PendingAdHocFilamentType>(*this);
    }

    /// \returns whether the filament name can be changed to \param new_name or a translatable error string.
    /// The filament must be customizable and the name must not collide (or it must be an ad-hoc filament)
    std::expected<void, const char *> can_be_renamed_to(const std::string_view &new_name) const;

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

constexpr FilamentType FilamentType::none = NoFilamentType {};
constexpr FilamentType FilamentType::pending_adhoc = PendingAdHocFilamentType {};
