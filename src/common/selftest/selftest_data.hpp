#pragma once

#include <variant>
#include <cstdint>
#include <type_traits>

#include "selftest_types.hpp"

namespace selftest {

struct FirstLayerCalibrationData {
    uint8_t previous_sheet;
    [[nodiscard]] constexpr uint32_t serialize() const {
        return previous_sheet;
    }

    [[nodiscard]] constexpr static FirstLayerCalibrationData deserialize(uint32_t raw_data) {
        return FirstLayerCalibrationData {
            .previous_sheet = static_cast<uint8_t>(raw_data),
        };
    }
};

template <typename Enum>
concept EnumUnderAndFourBytes = std::is_enum_v<Enum> && sizeof(std::underlying_type_t<Enum>) <= 4;

template <typename Struct>
concept TypeWithMemberSerialize = requires(Struct s) {
    { s.serialize() } -> std::same_as<uint32_t>;
};

template <typename T>
concept TypeWithStaticDeserialize = requires(uint32_t raw_data) {
    { T::deserialize(raw_data) } -> std::same_as<T>;
};

using TestData = std::variant<std::monostate, ToolMask, FirstLayerCalibrationData>;

uint32_t serialize_test_data_to_int(const TestData &test_data);
TestData deserialize_test_data_from_int(const uint8_t type_index, const uint32_t raw_data);

} // namespace selftest
