#pragma once

#include <filament.hpp>

struct EncodedFilamentType {

public:
    static constexpr uint8_t adhoc_filaments_offset = 176;
    static constexpr uint8_t user_filaments_offset = 192;

    static_assert(adhoc_filaments_offset + adhoc_filament_type_count <= user_filaments_offset);
    static_assert(static_cast<int>(user_filaments_offset) + max_user_filament_type_count <= 255);

    uint8_t data = 0;

public:
    constexpr EncodedFilamentType() = default;
    constexpr EncodedFilamentType(const EncodedFilamentType &) = default;

    constexpr EncodedFilamentType(const FilamentType &ft) {
        static constexpr auto visitor = []<typename T>(const T &v) -> uint8_t {
            if constexpr (std::is_same_v<T, PresetFilamentType>) {
                // 0 is for FilamentType::none
                return static_cast<uint8_t>(v) + 1;

            } else if constexpr (std::is_same_v<T, UserFilamentType>) {
                return v.index + user_filaments_offset;

            } else if constexpr (std::is_same_v<T, AdHocFilamentType>) {
                return v.tool + adhoc_filaments_offset;

            } else if constexpr (std::is_same_v<T, PendingAdHocFilamentType>) {
                // Should never get encoded
                assert(0);
                return 0;

            } else if constexpr (std::is_same_v<T, NoFilamentType>) {
                return 0;
            }
        };

        data = std::visit(visitor, ft);
    }

    static constexpr EncodedFilamentType from_data(uint8_t data) {
        EncodedFilamentType result;
        result.data = data;
        return result;
    }

    constexpr FilamentType decode() const {
        // 0 is for FilamentType::none
        if (data >= 1 && data < static_cast<uint8_t>(PresetFilamentType::_count) + 1) {
            return static_cast<PresetFilamentType>(data - 1);

        } else if (data >= user_filaments_offset && data < user_filaments_offset + user_filament_type_count) {
            return UserFilamentType { static_cast<uint8_t>(data - user_filaments_offset) };

        } else if (data >= adhoc_filaments_offset && data < adhoc_filaments_offset + adhoc_filament_type_count) {
            return AdHocFilamentType { static_cast<uint8_t>(data - adhoc_filaments_offset) };

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
