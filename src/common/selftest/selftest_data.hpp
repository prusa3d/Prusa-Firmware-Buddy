#pragma once

#include <variant>
#include <cstdint>
#include <type_traits>

#include "selftest_types.hpp"

namespace selftest {

template <typename Enum>
concept EnumUnderAndFourBytes = std::is_enum_v<Enum> && sizeof(std::underlying_type_t<Enum>) <= 4;

using TestData = std::variant<std::monostate, ToolMask>;

uint32_t serialize_test_data_to_int(const TestData &test_data);
TestData deserialize_test_data_from_int(const uint8_t type_index, const uint32_t raw_data);

} // namespace selftest
