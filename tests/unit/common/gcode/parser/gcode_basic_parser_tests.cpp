#include <catch2/catch.hpp>
#include <format>

#include "gcode_parser_test_common.hpp"

#include <gcode_basic_parser.hpp>

TEST_CASE("gcode_parser::gcode_basic_parser") {
    SECTION("1") {
        GCodeBasicParser p(fail_test_error_callback);
        CHECK(p.parse("  N1 G256 Test test *asd *163587 ;asd ;as"));
        CHECK(p.line_number() == 1);
        CHECK(p.command() == GCodeCommand { .letter = 'G', .codenum = 256 });
        CHECK(p.body() == "Test test");
    }

    SECTION("2") {
        GCodeBasicParser p(fail_test_error_callback);
        CHECK(p.parse("M1.12"));
        CHECK(p.line_number() == std::nullopt);
        CHECK(p.command() == GCodeCommand { .letter = 'M', .codenum = 1, .subcode = 12 });
        CHECK(p.body() == "");
    }

    SECTION("3") {
        CHECK(test_parse_failure<GCodeBasicParser>("Gx"));
        CHECK(test_parse_failure<GCodeBasicParser>("g1 S0"));
        CHECK(test_parse_failure<GCodeBasicParser>(""));
    }
}
