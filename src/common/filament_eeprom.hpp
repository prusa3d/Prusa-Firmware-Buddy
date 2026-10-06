#pragma once

#include "encoded_filament.hpp"

#include <utils/string/inplace_string.hpp>
#include <utils/color555.hpp>
#include <utils/compact_optional.hpp>
#include <uuid/uuid.hpp>

#include <option/has_chamber_api.h>
#include <option/has_filament_heatbreak_param.h>
#include <option/has_filament_base_preset_param.h>

// For historic reasons, the FilamentTypeParameters is split across multiple structures in the EEPROM

// !!! DO NOT CHANGE - this is used in config store
struct FilamentTypeParameters_EEPROM1 {

public:
    InplaceString<8> name;

    uint16_t nozzle_temperature;
    uint16_t nozzle_preheat_temperature = 170;
    uint8_t heatbed_temperature;

    bool requires_filtration : 1 = false;
    bool is_abrasive : 1 = false;
    bool is_flexible : 1 = false;

    // Keeping the remaining bits of the bitfield unused, but zero initialized, for future proofing
    uint8_t _unused : 5 = 0;

public:
    constexpr bool operator==(const FilamentTypeParameters_EEPROM1 &) const = default;
};
static_assert(sizeof(FilamentTypeParameters_EEPROM1) == 14);

#if HAS_CHAMBER_API()
// !!! DO NOT CHANGE - this is used in config store
struct __attribute__((packed)) FilamentTypeParameters_EEPROM2 {

public:
    static constexpr uint8_t chamber_temp_off = 255;

    uint8_t chamber_min_temperature = chamber_temp_off;
    uint8_t chamber_max_temperature = chamber_temp_off;
    uint8_t chamber_target_temperature = chamber_temp_off;

public:
    static constexpr std::optional<uint8_t> decode_chamber_temp(uint8_t val) {
        return val == chamber_temp_off ? std::optional<uint8_t>() : val;
    }
    static constexpr uint8_t encode_chamber_temp(std::optional<uint8_t> val) {
        return val.value_or(chamber_temp_off);
    }

    constexpr bool operator==(const FilamentTypeParameters_EEPROM2 &) const = default;
};
#endif

#if HAS_FILAMENT_HEATBREAK_PARAM()
// !!! DO NOT CHANGE - this is used in config store
struct __attribute__((packed)) FilamentTypeParameters_EEPROM3 {

public:
    uint8_t heatbreak_temperature = 35;

public:
    constexpr bool operator==(const FilamentTypeParameters_EEPROM3 &) const = default;
};
#endif

#if HAS_FILAMENT_BASE_PRESET_PARAM()
// !!! DO NOT CHANGE - this is used in config store
struct __attribute__((packed)) FilamentTypeParameters_EEPROM4 {

public:
    static constexpr uint8_t none_base_preset = 0xff;
    static_assert(std::to_underlying(PresetFilamentType::_count_sparse) < none_base_preset);
    uint8_t base_preset = none_base_preset;

    FilamentTypeParameters::BasePreset decode_base_preset() const {
        return EncodedFilamentType::preset_filament_type_from_enum_value(base_preset);
    }

    static uint8_t encode_base_preset(FilamentTypeParameters::BasePreset preset) {
        return preset.has_value() ? static_cast<uint8_t>(preset.value()) : none_base_preset;
    }

public:
    constexpr bool operator==(const FilamentTypeParameters_EEPROM4 &) const = default;
};
#endif

// !!! DO NOT CHANGE - this is used in config store
struct FilamentTypeParameters_EEPROM5 {

public:
    using EncodedColor = CompactOptional<Color555, Color555 { Color555::invalid_bit }>;
    EncodedColor encoded_color = std::nullopt;
    static_assert(sizeof(EncodedColor) == 2);

    static constexpr EncodedColor encode_color(OptionalColor color) {
        return color.transform(Color555::from_color);
    }

    constexpr OptionalColor color() const {
        return encoded_color.transform(&Color555::to_color);
    }

public:
    constexpr bool operator==(const FilamentTypeParameters_EEPROM5 &) const = default;
};

// !!! DO NOT CHANGE - this is used in config store
struct FilamentTypeParameters_EEPROM6 {

public:
    using OptionalUUID = CompactOptional<UUID, UUID::nil()>;
    using EncodedUUID = std::array<std::byte, 16>;

    static EncodedUUID encode_uuid(const OptionalUUID &uuid) {
        return uuid.value_or(UUID::nil()).bytes;
    }
    static OptionalUUID decode_uuid(EncodedUUID uuid) {
        return OptionalUUID {
            UUID::from_bytes(uuid),
            // Accept nil UUID as std::nullopt without shouting
            OptionalUUID::NulloptCheck::no
        };
    }

    EncodedUUID material_uuid {};
    EncodedUUID package_uuid {};
    EncodedUUID instance_uuid {};

public:
    constexpr bool operator==(const FilamentTypeParameters_EEPROM6 &) const = default;
};
