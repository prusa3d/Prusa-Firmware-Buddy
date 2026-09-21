#include "gcode_parser2_test_common.hpp"

#include <catch2/generators/catch_generators.hpp>

#include <filament.hpp>
#include <utils/color.hpp>
#include <utils/string_builder.hpp>

TEST_CASE("gcode_parser::custom_type::filament") {
    std::array<char, 64> buf;

    SECTION("1") {
        GCodeParser2 p(fail_test_error_callback);
        REQUIRE(p.parse("M1 C\"PETG\" A\"PLA\" G\"X\" O\"#\""));

        CHECK(p.option<std::string_view>('G', buf) == "X");
        CHECK(p.option<FilamentType>('G') == std::nullopt);

        CHECK(p.option<std::string_view>('C', buf) == "PETG");
        CHECK(p.option<FilamentType>('C') == PresetFilamentType::PETG);

        CHECK(p.option<std::string_view>('A', buf) == "PLA");
        CHECK(p.option<FilamentType>('A') == PresetFilamentType::PLA);

        CHECK(p.option<std::string_view>('O', buf) == "#");
        CHECK(p.option<FilamentType>('O') == PendingAdHocFilamentType());
    }
}

TEST_CASE("FilamentType::from_gcode_param") {
    SECTION("encoded round trip") {
        const FilamentType ft = GENERATE(
            FilamentType(PresetFilamentType::PETG),
            FilamentType(UserFilamentType { .index = 3 }),
            FilamentType(AdHocFilamentType { .tool = VirtualToolIndex::from_raw(0) }));

        ArrayStringBuilder<8> sb;
        ft.build_encoded_gcode_param(sb);
        CHECK(FilamentType::from_gcode_param(sb.str()) == ft);
    }

    SECTION("rejects") {
        CHECK(FilamentType::from_gcode_param("") == std::nullopt);
        CHECK(FilamentType::from_gcode_param("#0") == std::nullopt); // none
        CHECK(FilamentType::from_gcode_param("#256") == std::nullopt); // out of byte range
        CHECK(FilamentType::from_gcode_param("#1x") == std::nullopt); // trailing garbage
    }
}

TEST_CASE("gcode_parser::custom_type::color") {
    SECTION("1") {
        GCodeParser2 p(fail_test_error_callback);
        REQUIRE(p.parse("A1 A\"TERRACOTTA\" C\"#112233\" B0 D65535"));

        CHECK(p.option<Color>('A') == Color::from_raw(0xB87F6A));
        CHECK(p.option<Color>('B') == COLOR_BLACK);
        CHECK(p.option<Color>('C') == Color::from_rgb(0x11, 0x22, 0x33));
        CHECK(p.option<Color>('D') == Color::from_rgb(0, 255, 255));
    }
}
