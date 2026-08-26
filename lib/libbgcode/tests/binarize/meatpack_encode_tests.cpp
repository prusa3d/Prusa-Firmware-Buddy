#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "binarize_test_utils.hpp"

#include <string>

using namespace bgcode::core;
using namespace bgcode::binarize;
using namespace bgcode_test;

namespace {

// Round-trip raw_data through GCodeBlock write and read, returning what was read
// back. The write must succeed.
std::string roundtrip_gcode_block(EGCodeEncodingType encoding, const std::string& raw_data) {
    GCodeBlock read_block;
    REQUIRE(roundtrip_block(make_gcode_block(encoding, raw_data), ECompressionType::None, read_block) == EResult::Success);
    return read_block.raw_data;
}

// The result of encoding raw_data as a GCodeBlock, without asserting success, so
// a caller can check for an expected encoding failure.
EResult gcode_block_write_result(EGCodeEncodingType encoding, const std::string& raw_data) {
    ScopedFile file;
    return write_block_to(*file, make_gcode_block(encoding, raw_data), ECompressionType::None);
}

} // namespace

// A literal 0x00 must travel through MeatPack's verbatim-escape path. The
// payloads use an explicit length so the embedded NUL is not truncated, and end
// in '\n' so the codec's trailing-newline normalisation does not perturb the
// comparison. They place 0x00 in both escape-nibble positions: next to another
// unpackable byte, and immediately after a packable one.
TEST_CASE("MeatPack encode carries a literal 0x00 byte", "[Binarize][MeatPack]") {
    const EGCodeEncodingType encoding = GENERATE(EGCodeEncodingType::MeatPack, EGCodeEncodingType::MeatPackComments);
    CAPTURE(encoding);

    const std::string beside_unpackable("z\x00z\n", 4);
    const std::string after_packable("0\x00z\n", 4);

    REQUIRE(roundtrip_gcode_block(encoding, beside_unpackable) == beside_unpackable);
    REQUIRE(roundtrip_gcode_block(encoding, after_packable) == after_packable);
}

// 0xFF is MeatPack's signal byte and cannot be represented, so encoding must
// reject it rather than silently corrupt the stream - including when it is mixed
// with a byte the encoder can carry (0x00).
TEST_CASE("MeatPack encode rejects a literal 0xFF byte", "[Binarize][MeatPack]") {
    const std::string payload("z\xffz\n", 4);
    const std::string mixed_with_nul("z\x00\xffz\n", 5);

    REQUIRE(gcode_block_write_result(EGCodeEncodingType::MeatPack, payload) == EResult::GCodeEncodingError);
    REQUIRE(gcode_block_write_result(EGCodeEncodingType::MeatPackComments, payload) == EResult::GCodeEncodingError);
    REQUIRE(gcode_block_write_result(EGCodeEncodingType::MeatPack, mixed_with_nul) == EResult::GCodeEncodingError);
}

// A full comment line is kept verbatim under MeatPackComments, so a 0xFF in it
// reaches the wire and must be rejected. Under MeatPack the line is stripped
// entirely, so the same input is accepted.
TEST_CASE("MeatPack encode rejects a 0xFF in a kept comment, accepts it in a stripped one", "[Binarize][MeatPack]") {
    const std::string comment("; x\xff\n", 5);

    REQUIRE(gcode_block_write_result(EGCodeEncodingType::MeatPackComments, comment) == EResult::GCodeEncodingError);
    REQUIRE(gcode_block_write_result(EGCodeEncodingType::MeatPack, comment) == EResult::Success);
}

// An inline comment (after ';' on a code line) is stripped in both modes, so a
// 0xFF confined to it never reaches the wire and is accepted in both.
TEST_CASE("MeatPack encode accepts a 0xFF byte inside an inline comment", "[Binarize][MeatPack]") {
    const std::string inline_comment("G1 X1 ;\xff\n", 9);

    REQUIRE(gcode_block_write_result(EGCodeEncodingType::MeatPack, inline_comment) == EResult::Success);
    REQUIRE(gcode_block_write_result(EGCodeEncodingType::MeatPackComments, inline_comment) == EResult::Success);
}

// Control against over-rejection: 0xFA is not the signal byte and travels
// through the verbatim-escape path, so it must still round-trip.
TEST_CASE("MeatPack encode accepts other high bytes", "[Binarize][MeatPack]") {
    const std::string payload("z\xfaz\n", 4);

    REQUIRE(roundtrip_gcode_block(EGCodeEncodingType::MeatPack, payload) == payload);
    REQUIRE(roundtrip_gcode_block(EGCodeEncodingType::MeatPackComments, payload) == payload);
}

// Control proving the rejection is specific to MeatPack: under
// EGCodeEncodingType::None both 0x00 and 0xFF are copied verbatim and round-trip.
TEST_CASE("encoding None stores 0x00 and 0xFF verbatim", "[Binarize][MeatPack]") {
    const std::string payload("z\x00\xffz\n", 5);

    REQUIRE(roundtrip_gcode_block(EGCodeEncodingType::None, payload) == payload);
}
