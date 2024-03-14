/// gcode_process tests

#include <algorithm>
#include <cstring>
#include <vector>

#include "catch2/catch.hpp"

#include "common/gcode/gcode_filter.hpp"

#define MAX_CMD_SIZE  96
#define LOOP_MAX_READ 50

using Catch::Matchers::Equals;

static const char input_gcode[] = "G1\r\n"
                                  "G2 ; with a comment\n\r"
                                  "\n" // Empty line
                                  "; Just a comment\n"
                                  "; G3 G-Code within a comment\n"
                                  "  G4\n" // G-Code with leading spaces
                                  // G-Code with too many leading spaces
                                  "                                                                                                G5\n"
                                  // G-Code with too many trailing spaces
                                  "G6                                                                                                \n"
                                  "G7 S1; G-Code with a parameter\n"
                                  "; too long comment with a G-Code                                                                G8\n"
                                  // Line with too many commented out G-codes
                                  "; G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9 G9\n"
                                  "G10" // The last G-Code without trailing new line
    ;

static const std::vector<const char *> expected_gcode = {
    "G1",
    "G2",
    "G4",
    "G5",
    "G6",
    "G7 S1",
    "G10"
};

static const size_t gcode_size = sizeof(input_gcode);

char buffer[MAX_CMD_SIZE + 1];

unsigned int processed_chars = 0;
unsigned int previous_processed_chars = 0;
unsigned int loop_read = 0;

char getByte(GCodeFilter::State *state) {
    if (loop_read == LOOP_MAX_READ) {
        *state = GCodeFilter::State::Timeout;
        return '\0';
    }
    if (processed_chars == gcode_size) {
        *state = GCodeFilter::State::Eof;
        return '\0';
    }
    loop_read++;
    *state = GCodeFilter::State::Ok;
    return input_gcode[processed_chars++];
}

TEST_CASE("gcode-filter", "") {
    unsigned int gcode_index = 0;
    GCodeFilter gcode_filter(&getByte, buffer, sizeof(buffer));
    GCodeFilter::State state;
    for (char *gcode = gcode_filter.nextGcode(&state);
         state != GCodeFilter::State::Error;
         gcode = gcode_filter.nextGcode(&state)) {

        // Check the Skip functionality (unlocking loop after specified size read)
        CHECK(loop_read <= LOOP_MAX_READ);
        if (loop_read == LOOP_MAX_READ) {
            CHECK(state == GCodeFilter::State::Timeout);
            CHECK(gcode == nullptr);
            previous_processed_chars = processed_chars;
            loop_read = 0;
            continue;
        }
        CHECK(processed_chars - previous_processed_chars == loop_read);
        previous_processed_chars = processed_chars;
        loop_read = 0;

        // Check the gcode
        REQUIRE(gcode != nullptr);
        REQUIRE(gcode_index < expected_gcode.size());
        CHECK_THAT(gcode, Equals(expected_gcode[gcode_index++]));

        if (state == GCodeFilter::State::Eof) {
            break;
        }
    }
    // Make sure the loop didn't end with an error
    CHECK(state == GCodeFilter::State::Eof);
    // Make sure we have processed every char
    CHECK(processed_chars == gcode_size);
    // Make sure we have checked every line
    CHECK(gcode_index == expected_gcode.size());
}
