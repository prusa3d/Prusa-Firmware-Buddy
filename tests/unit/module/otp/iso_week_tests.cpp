#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

#include <otp/utils.hpp>

#include <algorithm>
#include <array>
#include <cstring>

TEST_CASE("decode_datamatrix production date matches the OTP test vectors", "[otp][iso_week]") {
    // Same datamatrix strings as the week-format cases in tests/unit/common/otp_test.cpp
    const auto [text, expected_year, expected_month, expected_day] = GENERATE(table<const char *, uint16_t, uint8_t, uint8_t>({
        { "14558-0200001952299999", 2025, 5, 29 },
        { "14558-0200001902299999", 2020, 5, 28 },
    }));

    std::array<uint8_t, 24> buffer {};
    std::copy_n(text, std::strlen(text), buffer.begin());

    const auto decoded = otp::decode_datamatrix(std::span<const uint8_t, 24> { buffer });

    INFO("datamatrix " << text);
    REQUIRE(decoded.has_value());
    REQUIRE(decoded->production_year == expected_year);
    REQUIRE(decoded->production_month == expected_month);
    REQUIRE(decoded->production_day == expected_day);
}
