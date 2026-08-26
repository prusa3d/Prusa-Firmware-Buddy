#include <catch2/catch_test_macros.hpp>

#include "binarize_test_utils.hpp"

#include <cstdint>
#include <vector>

using namespace bgcode::core;
using namespace bgcode::binarize;
using namespace bgcode_test;

// An empty block writes both sizes as zero; deflate used to reject reading it
// back while None and heatshrink accepted it. All four must now agree, for gcode
// and metadata blocks alike (both reach the same uncompress path).
TEST_CASE("an empty block round-trips under every compression type", "[Binarize][Decompress]") {
    struct NamedCompression {
        const char* name;
        ECompressionType type;
    };
    const NamedCompression compressions[] = {
        {"None", ECompressionType::None},
        {"Deflate", ECompressionType::Deflate},
        {"Heatshrink_11_4", ECompressionType::Heatshrink_11_4},
        {"Heatshrink_12_4", ECompressionType::Heatshrink_12_4},
    };

    for (const NamedCompression& compression : compressions) {
        INFO("compression type: " << compression.name);

        const GCodeBlock gcode_block = make_gcode_block(EGCodeEncodingType::None, "");
        GCodeBlock read_gcode_block;
        REQUIRE(roundtrip_block(gcode_block, compression.type, read_gcode_block) == EResult::Success);
        REQUIRE(read_gcode_block.raw_data.empty());

        const FileMetadataBlock metadata_block; // empty raw_data, INI encoding
        FileMetadataBlock read_metadata_block;
        REQUIRE(roundtrip_block(metadata_block, compression.type, read_metadata_block) == EResult::Success);
        REQUIRE(read_metadata_block.raw_data.empty());
    }
}

// The other accepted form: a non-empty deflate stream that legitimately encodes
// empty output (declaring uncompressed_size zero) must still decode, unchanged by
// the both-zero short-circuit.
TEST_CASE("a non-empty deflate stream encoding empty output still decodes", "[Binarize][Decompress]") {
    const std::vector<uint8_t> empty_deflate_stream = {0x78, 0x9c, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01};

    const ScopedFile file = make_block_file(EBlockType::FileMetadata, ECompressionType::Deflate,
                                            static_cast<uint16_t>(EMetadataEncodingType::INI),
                                            /*uncompressed_size*/ 0,
                                            static_cast<uint32_t>(empty_deflate_stream.size()),
                                            empty_deflate_stream);

    FileMetadataBlock block;
    REQUIRE(read_first_block(*file, block) == EResult::Success);
    REQUIRE(block.raw_data.empty());
}
