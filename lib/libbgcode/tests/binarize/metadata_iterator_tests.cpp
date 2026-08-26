#include <catch2/catch_test_macros.hpp>

#include "binarize_test_utils.hpp"

#include <cstdint>
#include <string>
#include <utility>

using namespace bgcode::core;
using namespace bgcode::binarize;
using namespace bgcode_test;

namespace {

FileMetadataBlock read_ini_metadata(const std::string &payload, EResult &result) {
    const ScopedFile file = make_block_file(EBlockType::FileMetadata, ECompressionType::None,
                                            static_cast<uint16_t>(EMetadataEncodingType::INI),
                                            static_cast<uint32_t>(payload.size()), 0, payload);
    FileMetadataBlock block;
    result = read_first_block(*file, block);
    return block;
}

} // namespace

// A metadata line that carries no '=' used to leave the decoder's iterator where
// it was, so the loop rebuilt the same empty item forever. An ordinary INI
// comment or blank line is enough to reach it, so a well-formed file could hang
// the reader. The decoder must now skip such lines and terminate.
TEST_CASE("decode_metadata terminates on a line without '='", "[Binarize][Metadata]") {
    EResult result = EResult::Success;
    const FileMetadataBlock block = read_ini_metadata("noequals\na=1\n", result);

    REQUIRE(result == EResult::Success);
    REQUIRE(block.raw_data.size() == 1);
    REQUIRE(block.raw_data.front() == std::make_pair(std::string("a"), std::string("1")));
}

// A final line with '=' and no trailing newline used to push the iterator one
// past the end and then scan beyond the buffer for a newline. The whole payload
// is exactly one key with no terminator, so a correct decoder yields exactly one
// pair and reads nothing past the end.
TEST_CASE("decode_metadata does not read past an unterminated final line", "[Binarize][Metadata]") {
    EResult result = EResult::Success;
    const FileMetadataBlock block = read_ini_metadata("a=1", result);

    REQUIRE(result == EResult::Success);
    REQUIRE(block.raw_data.size() == 1);
    REQUIRE(block.raw_data.front() == std::make_pair(std::string("a"), std::string("1")));
}

// The same off-by-one lived on the encode side: MeatPack encoding advanced past
// the newline unconditionally, so a final line without one stepped past the end
// and the next scan walked memory. Encoding gcode whose last line has no newline
// must succeed and round-trip unchanged.
TEST_CASE("encode_gcode handles a final line without a trailing newline", "[Binarize][MeatPack]") {
    const GCodeBlock block = make_gcode_block(EGCodeEncodingType::MeatPack, "G1 X1 Y1 Z1"); // no trailing '\n'

    GCodeBlock read_block;
    REQUIRE(roundtrip_block(block, ECompressionType::None, read_block) == EResult::Success);

    // The codec terminates the decoded gcode with a newline; the content itself
    // must survive intact rather than being read past the end and corrupted.
    REQUIRE(read_block.raw_data == block.raw_data + "\n");
}
