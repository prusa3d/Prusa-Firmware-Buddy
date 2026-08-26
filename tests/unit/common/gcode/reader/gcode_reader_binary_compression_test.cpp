#include <gcode_reader_binary.hpp>
#include <unique_file_ptr.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <sys/stat.h>

namespace {

// Opens a tmp file and writes the bgcode FileHeader followed by the given
// (already filled in) block header. Leaves the file positioned right after
// the header, ready for the caller to add the block's payload.
unique_file_ptr make_file_with_block_header(bgcode::core::BlockHeader &block_header) {
    unique_file_ptr file(std::tmpfile());
    REQUIRE(file.get() != nullptr);

    bgcode::core::FileHeader file_header;
    REQUIRE(file_header.write(*file) == bgcode::core::EResult::Success);
    REQUIRE(block_header.write(*file) == bgcode::core::EResult::Success);

    return file;
}

// Writes a minimal bgcode file containing just the given (already filled in)
// thumbnail block header, followed by its ThumbnailParams payload.
unique_file_ptr make_thumbnail_file(bgcode::core::BlockHeader &block_header) {
    auto file = make_file_with_block_header(block_header);

    bgcode::core::ThumbnailParams thumb_params {
        .format = static_cast<uint16_t>(bgcode::core::EThumbnailFormat::PNG),
        .width = 64,
        .height = 64,
    };
    REQUIRE(thumb_params.write(*file) == bgcode::core::EResult::Success);

    rewind(file.get());
    return file;
}

// Writes a bgcode file containing the given (already filled in) metadata
// block header, an INI encoding tag as its payload, and a trailing empty
// GCode block so the reader terminates the scan normally.
unique_file_ptr make_metadata_file(bgcode::core::BlockHeader &block_header) {
    auto file = make_file_with_block_header(block_header);

    const uint16_t metadata_encoding = static_cast<uint16_t>(bgcode::core::EMetadataEncodingType::INI);
    REQUIRE(fwrite(&metadata_encoding, sizeof(metadata_encoding), 1, file.get()) == 1);

    bgcode::core::BlockHeader gcode_block_header(
        static_cast<uint16_t>(bgcode::core::EBlockType::GCode),
        static_cast<uint16_t>(bgcode::core::ECompressionType::None),
        /* uncompressed_size */ 0,
        /* compressed_size */ 0);
    REQUIRE(gcode_block_header.write(*file) == bgcode::core::EResult::Success);

    const uint16_t gcode_encoding = static_cast<uint16_t>(bgcode::core::EGCodeEncodingType::None);
    REQUIRE(fwrite(&gcode_encoding, sizeof(gcode_encoding), 1, file.get()) == 1);

    rewind(file.get());
    return file;
}

} // namespace

// A block header that claims heatshrink compression, yet a compressed_size of
// 0, is a contradiction: it never occurs in a well-formed file (an
// uncompressed block is the only one legitimately carrying compressed_size ==
// 0). The reader must not treat such a block as plain data.
TEST_CASE("Thumbnail block: compression field takes precedence over compressed_size", "[GcodeReader]") {
    bgcode::core::BlockHeader block_header(
        static_cast<uint16_t>(bgcode::core::EBlockType::Thumbnail),
        static_cast<uint16_t>(bgcode::core::ECompressionType::Heatshrink_11_4),
        /* uncompressed_size */ 100,
        /* compressed_size */ 0);

    auto file = make_thumbnail_file(block_header);

    struct stat st = {};
    REQUIRE(fstat(fileno(file.get()), &st) == 0);

    PrusaPackGcodeReader reader(std::move(file), st);
    // Must not accept the block as a readable, uncompressed thumbnail.
    REQUIRE(reader.stream_thumbnail_start(64, 64, IGcodeReader::ImgType::PNG, false) == nullptr);
}

TEST_CASE("Metadata block: compression field takes precedence over compressed_size", "[GcodeReader]") {
    bgcode::core::BlockHeader block_header(
        static_cast<uint16_t>(bgcode::core::EBlockType::PrinterMetadata),
        static_cast<uint16_t>(bgcode::core::ECompressionType::Heatshrink_11_4),
        /* uncompressed_size */ 100,
        /* compressed_size */ 0);

    auto file = make_metadata_file(block_header);

    struct stat st = {};
    REQUIRE(fstat(fileno(file.get()), &st) == 0);

    PrusaPackGcodeReader reader(std::move(file), st);
    IGcodeReader::Index index;
    reader.generate_index(index, false);
    // Must not index the block as readable metadata.
    REQUIRE(index.metadata == IGcodeReader::Index::not_present);
}
