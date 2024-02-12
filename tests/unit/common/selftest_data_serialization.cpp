#include "catch2/catch.hpp"

#include <array>

#include <common/selftest/selftest_data.hpp>

TEST_CASE("selftest::TestData ToolMask serialization/deserialization", "[selftest][serialization][variant]") {
    using namespace selftest;

    const ToolMask tm = GENERATE(ToolMask::NoneTools, ToolMask::AllTools, ToolMask::Tool1);
    const TestData td = tm;
    const auto intermediate = serialize_test_data_to_int(td);

    const auto deserialized_test_data = deserialize_test_data_from_int(td.index(), intermediate);

    REQUIRE(std::holds_alternative<ToolMask>(deserialized_test_data));
    REQUIRE(std::get<ToolMask>(deserialized_test_data) == tm);
}

TEST_CASE("selftest::TestData FirstLayerCalibrationData serialization/deserialization", "[selftest][serialization][variant]") {
    using namespace selftest;

    const auto sheet_index = GENERATE(range(0, 16));
    const TestData td = FirstLayerCalibrationData { .previous_sheet = sheet_index };

    const auto intermediate = serialize_test_data_to_int(td);

    const auto deserialized_test_data = deserialize_test_data_from_int(td.index(), intermediate);

    REQUIRE(std::holds_alternative<FirstLayerCalibrationData>(deserialized_test_data));
    REQUIRE(std::get<FirstLayerCalibrationData>(deserialized_test_data).previous_sheet == sheet_index);
}
