#include "gcode_parser2_test_common.hpp"

#include <filament.hpp>
#include <utils/color.hpp>

TEST_CASE("gcode_parser::custom_type::filament") {
    std::array<char, 64> buf;

    SECTION("1") {
        GCodeParser2 p(fail_test_error_callback);
        REQUIRE(p.parse("M1 C\"PETG\" A\"PLA\" G\"X\" O\"#2\""));

        CHECK(p.option<std::string_view>('G', buf) == "X");
        CHECK(p.option<FilamentType>('G') == std::nullopt);

        CHECK(p.option<std::string_view>('C', buf) == "PETG");
        CHECK(p.option<FilamentType>('C') == PresetFilamentType::PETG);

        CHECK(p.option<std::string_view>('A', buf) == "PLA");
        CHECK(p.option<FilamentType>('A') == PresetFilamentType::PLA);

        CHECK(p.option<std::string_view>('O', buf) == "#2");
        CHECK(p.option<FilamentType>('O') == AdHocFilamentType(2));
    }
}
