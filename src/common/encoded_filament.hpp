#pragma once

#include <utility>
#include <bitset>

#include <filament.hpp>
#include <bsod/bsod.h>
#include <utils/overloaded_visitor.hpp>

struct EncodedFilamentType {

public:
    static constexpr uint8_t adhoc_filaments_offset = 176;
    static constexpr uint8_t user_filaments_offset = 192;
    static constexpr uint8_t pending_adhoc_filament_value = 175;

    static_assert(std::to_underlying(PresetFilamentType::_count_sparse) + 1 /* no filament */ <= pending_adhoc_filament_value);
    static_assert(pending_adhoc_filament_value < adhoc_filaments_offset);
    static_assert(adhoc_filaments_offset + adhoc_filament_type_count <= user_filaments_offset);
    static_assert(user_filaments_offset + max_user_filament_type_count <= 255);

    uint8_t data = 0;

public:
    constexpr EncodedFilamentType() = default;
    constexpr EncodedFilamentType(const EncodedFilamentType &) = default;

    constexpr EncodedFilamentType(const FilamentType &ft) {
        data = match(
            ft, //
            // +1 -> 0 is for FilamentType::none
            [](PresetFilamentType v) -> uint8_t { return static_cast<uint8_t>(v) + 1; }, //
            [](UserFilamentType v) -> uint8_t { return v.index + user_filaments_offset; }, //
            [](AdHocFilamentType v) -> uint8_t { return v.tool.to_raw() + adhoc_filaments_offset; }, //
            [](PendingAdHocFilamentType) -> uint8_t { return pending_adhoc_filament_value; }, //
            [](NoFilamentType) -> uint8_t { return 0; } //
        );
    }

    static constexpr EncodedFilamentType from_data(uint8_t data) {
        EncodedFilamentType result;
        result.data = data;
        return result;
    }

    /// Decodes PresetFilamentType from index (index is not the same as .encode()!)
    static constexpr std::optional<PresetFilamentType> preset_filament_type_from_enum_value(uint8_t index) {
        static constexpr auto valid_bitset = [] {
            std::bitset<std::to_underlying(PresetFilamentType::_count_sparse)> result;
            for (auto t : preset_filament_types) {
                result.set(std::to_underlying(t));
            }
            return result;
        }();

        if (index < std::to_underlying(PresetFilamentType::_count_sparse) && valid_bitset.test(index)) {
            return static_cast<PresetFilamentType>(index);
        } else {
            return std::nullopt;
        }
    }

    constexpr FilamentType decode() const {
        // 0 is for FilamentType::none
        if (const auto i = data - 1; data >= 1 && i < static_cast<uint8_t>(PresetFilamentType::_count_sparse)) {
            return FilamentType::from_optional(preset_filament_type_from_enum_value(i));

        } else if (data >= user_filaments_offset && data < user_filaments_offset + user_filament_type_count) {
            return UserFilamentType { static_cast<uint8_t>(data - user_filaments_offset) };

        } else if (data >= adhoc_filaments_offset && data < adhoc_filaments_offset + adhoc_filament_type_count) {
            // Note: bounds checked by adhoc_filament_type_count
            return AdHocFilamentType { VirtualToolIndex::from_raw(static_cast<uint8_t>(data - adhoc_filaments_offset)) };

        } else if (data == pending_adhoc_filament_value) {
            return PendingAdHocFilamentType {};

        } else {
            return NoFilamentType {};
        }
    }

public:
    inline constexpr bool operator==(const EncodedFilamentType &) const = default;
    inline constexpr bool operator!=(const EncodedFilamentType &) const = default;

    inline constexpr operator FilamentType() const {
        return decode();
    }
};
