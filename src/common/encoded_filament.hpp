#pragma once

#include <filament.hpp>

struct EncodedFilamentType {

public:
    uint8_t data = 0;

public:
    constexpr EncodedFilamentType() = default;
    constexpr EncodedFilamentType(const EncodedFilamentType &) = default;

    constexpr EncodedFilamentType(const FilamentType &ft) {
        static constexpr auto visitor = []<typename T>(const T &v) -> uint8_t {
            if constexpr (std::is_same_v<T, PresetFilamentType>) {
                // 0 is for FilamentType::none
                return static_cast<uint8_t>(v) + 1;

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
        if (data == 0) {
            return FilamentType();

        } else {
            // 0 is for FilamentType::none
            return static_cast<PresetFilamentType>(data - 1);
        }
    }

public:
    inline constexpr bool operator==(const EncodedFilamentType &) const = default;
    inline constexpr bool operator!=(const EncodedFilamentType &) const = default;

    inline constexpr operator FilamentType() const {
        return decode();
    }
};
