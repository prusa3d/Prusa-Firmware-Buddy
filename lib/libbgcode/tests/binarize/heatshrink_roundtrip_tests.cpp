#include <catch2/catch_test_macros.hpp>

#include "binarize_test_utils.hpp"

#include <string>

using namespace bgcode::core;
using namespace bgcode::binarize;
using namespace bgcode_test;

// The heatshrink compressor could drop part of its output and write a block that
// does not decode back to its input; short payloads were enough. A round trip of
// a small payload through heatshrink must reproduce it exactly.
TEST_CASE("heatshrink write/read round-trips a short payload", "[Binarize][Decompress]") {
    for (const std::string &payload : {std::string("G"), std::string("G1"), std::string("G1\n")}) {
        const GCodeBlock block = make_gcode_block(EGCodeEncodingType::None, payload);

        GCodeBlock read_block;
        REQUIRE(roundtrip_block(block, ECompressionType::Heatshrink_11_4, read_block) == EResult::Success);
        REQUIRE(read_block.raw_data == payload);
    }
}
