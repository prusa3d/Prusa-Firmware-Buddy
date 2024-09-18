#include <catch2/catch.hpp>

#include <format>
#include <functional>

#include <gcode_parser.hpp>

#include "gcode_parser_test_common.hpp"

std::string option_list(GCodeParser2 &p) {
    std::string result;
    for (int ch = 0; ch < 256; ch++) {
        if (p.has_option(static_cast<char>(ch))) {
            result.push_back(static_cast<char>(ch));
        }
    }
    return result;
}

TEST_CASE("gcode_parser::gcode_parser::params_tests") {
    std::array<char, 64> buf;

    SECTION("1") {
        GCodeParser2 p(fail_test_error_callback);
        REQUIRE(p.parse("  N1 G256 X0 Z1Y3F ; O1"));
        CHECK(p.line_number() == 1);
        CHECK(p.command() == GCodeCommand { .letter = 'G', .codenum = 256 });
        CHECK(p.body() == "X0 Z1Y3F");
        CHECK(option_list(p) == "FXYZ");

        CHECK(!p.option<bool>('A').has_value());

        CHECK(p.option<bool>('X') == false);
        CHECK(p.option<int>('X') == 0);
        CHECK(p.option<float>('X') == 0);
        CHECK(p.option<std::string_view>('X', buf) == "0");

        CHECK(p.option<bool>('Z') == true);
        CHECK(p.option<int>('Z') == 1);
        CHECK(p.option<float>('Z') == 1);
        CHECK(p.option<std::string_view>('Z', buf) == "1");

        CHECK(p.option<int>('Y') == 3);
        CHECK(p.option<float>('Y') == 3);
        CHECK(p.option<std::string_view>('Y', buf) == "3");
        CHECK(execution_failed(p, [&] { std::ignore = p.option<bool>('Y'); }));

        CHECK(p.option<bool>('F') == true);
        CHECK(execution_failed(p, [&] { std::ignore = p.option<int>('F'); }));
        CHECK(p.option<std::string_view>('F', buf) == "");

        CHECK(p.option<std::string_view>('G', buf) == std::unexpected(GCodeParser2::OptionError::not_present));
    }

    SECTION("2") {
        GCodeParser2 p(fail_test_error_callback);
        REQUIRE(p.parse("X999 X\" Y\" Z"));
        CHECK(p.command() == GCodeCommand { .letter = 'X', .codenum = 999 });
        CHECK(option_list(p) == "XZ");

        CHECK(p.option<std::string_view>('X', buf) == " Y");
        CHECK(p.option<std::string_view>('Z', buf) == "");
    }

    SECTION("3") {
        GCodeParser2 p(fail_test_error_callback);
        REQUIRE(p.parse("M13 X\"TESTXYZ\" Y \"ZYXEST\" Z   \"KAKA \\\"LOL\" *123 ZYXXA"));
        CHECK(p.command() == GCodeCommand { .letter = 'M', .codenum = 13 });
        CHECK(option_list(p) == "XYZ");

        CHECK(p.option<std::string_view>('X', buf) == "TESTXYZ");

        CHECK(p.option<std::string_view>('Y', buf) == "ZYXEST");
        CHECK(execution_failed(p, [&] { std::ignore = p.option<int>('Y'); }));
        CHECK(execution_failed(p, [&] { std::ignore = p.option<bool>('Y'); }));

        CHECK(p.option<std::string_view>('Z', buf) == "KAKA \"LOL");
    }

    SECTION("4") {
        GCodeParser2 p(fail_test_error_callback);
        REQUIRE(p.parse("G0 ABCDE1G0HF"));
        CHECK(p.command() == GCodeCommand { .letter = 'G', .codenum = 0 });
        CHECK(option_list(p) == "ABCDEFGH");

        CHECK(p.option<std::string_view>('A', buf) == "");
        CHECK(p.option<bool>('A') == true);
        CHECK(p.option<bool>('B') == true);
        CHECK(p.option<bool>('C') == true);
        CHECK(p.option<bool>('D') == true);
        CHECK(p.option<bool>('E') == true);
        CHECK(p.option<bool>('F') == true);
        CHECK(p.option<bool>('G') == false);
        CHECK(p.option<bool>('H') == true);
        CHECK(p.option<bool>('I') == std::unexpected(GCodeParser2::OptionError::not_present));
    }

    SECTION("escaping_test") {
        GCodeParser2 p(fail_test_error_callback);
        REQUIRE(p.parse("Z12 A\"TES\\\"XES\" B\"ZAB\\\\ac\\\"AS\""));
        CHECK(p.command() == GCodeCommand { .letter = 'Z', .codenum = 12 });
        CHECK(option_list(p) == "AB");

        CHECK(p.option<std::string_view>('A', buf) == "TES\"XES");
        CHECK(p.option<std::string_view>('B', buf) == "ZAB\\ac\"AS");
    }
}

TEST_CASE("gcode_parser::gcode_parser::failure_tests") {
    // Check unclosed quotes
    CHECK(test_parse_failure<GCodeParser2>("G13 X\""));
    CHECK(test_parse_failure<GCodeParser2>("G13 X\" F"));
    CHECK(test_parse_failure<GCodeParser2>("G13 X\";\""));
    CHECK(test_parse_failure<GCodeParser2>("G13 X\";*\""));
    CHECK(test_parse_failure<GCodeParser2>("G13 X\"*;ASD\""));

    // Check nonsense option data that's not quoted
    CHECK(test_parse_failure<GCodeParser2>("G13 Xx Yy"));
    CHECK(test_parse_failure<GCodeParser2>("G13 X123blbost Y"));
    CHECK(test_parse_failure<GCodeParser2>("G13 X%23"));
}
