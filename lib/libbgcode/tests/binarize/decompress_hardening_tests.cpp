#include <catch2/catch_test_macros.hpp>

#include "binarize_test_utils.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace bgcode::core;
using namespace bgcode::binarize;
using namespace bgcode_test;

namespace {

// A complete deflate stream of "k=v\n".
const std::vector<uint8_t> kDeflateKV = {
    0x78, 0xda, 0xcb, 0xb6, 0x2d, 0xe3, 0x02, 0x00, 0x03, 0x5d, 0x01, 0x29,
};

// A complete deflate stream of the 16-byte INI payload "key=value\nk2=v2\n".
const std::vector<uint8_t> kDeflateIni16 = {
    0x78, 0xda, 0xcb, 0x4e, 0xad, 0xb4, 0x2d, 0x4b, 0xcc, 0x29, 0x4d, 0xe5,
    0xca, 0x36, 0xb2, 0x2d, 0x33, 0xe2, 0x02, 0x00, 0x32, 0x18, 0x05, 0x3a,
};

// Read a FileMetadata block crafted from raw bytes with explicit, possibly
// dishonest, declared sizes -- the whole point of these cases is to make the
// declared sizes disagree with what is actually present.
EResult read_crafted_metadata(ECompressionType compression, uint32_t uncompressed_size,
                              uint32_t compressed_size, const std::vector<uint8_t> &payload,
                              FileMetadataBlock &block) {
    const ScopedFile file = make_block_file(EBlockType::FileMetadata, compression,
                                            static_cast<uint16_t>(EMetadataEncodingType::INI),
                                            uncompressed_size, compressed_size, payload);
    return read_first_block(*file, block);
}

// Restores the global block-size limit whatever a test does to it.
struct ScopedBlockSizeLimit {
    size_t previous;
    explicit ScopedBlockSizeLimit(size_t limit) : previous(get_max_block_data_size()) {
        set_max_block_data_size(limit);
    }
    ~ScopedBlockSizeLimit() { set_max_block_data_size(previous); }
};

} // namespace

// A complete deflate stream followed by trailing bytes used to spin the inflate
// loop forever: it drove avail_in above zero while inflate made no further
// progress. It must now stop on the finished stream and still return the real
// payload.
TEST_CASE("uncompress terminates on bytes trailing a deflate stream", "[Binarize][Decompress]") {
    std::vector<uint8_t> payload = kDeflateKV;
    payload.insert(payload.end(), 16, 0x00); // trailing garbage after a complete stream

    FileMetadataBlock block;
    const EResult result = read_crafted_metadata(ECompressionType::Deflate, /*uncompressed*/ 4,
                                                 static_cast<uint32_t>(payload.size()), payload, block);

    REQUIRE(result == EResult::Success);
    REQUIRE(block.raw_data.size() == 1);
    REQUIRE(block.raw_data.front() == std::make_pair(std::string("k"), std::string("v")));
}

// A heatshrink block that carries more input than fits the decoder's internal
// buffer, while declaring a tiny output size, must not deadlock.
TEST_CASE("uncompress terminates on a stalling heatshrink stream", "[Binarize][Decompress]") {
    std::vector<uint8_t> payload(4096);
    for (size_t i = 0; i < payload.size(); ++i)
        payload[i] = static_cast<uint8_t>((i * 37 + 11) & 0xFF);

    FileMetadataBlock block;
    const EResult result = read_crafted_metadata(ECompressionType::Heatshrink_11_4, /*uncompressed*/ 16,
                                                 static_cast<uint32_t>(payload.size()), payload, block);

    REQUIRE(result != EResult::Success);
}

// Deflate treated uncompressed_size as a mere reserve() hint, so a stream that
// really yields 16 bytes while the header declares 8 was accepted and handed the
// caller data whose length contradicted the header. The mismatch must now be
// rejected.
TEST_CASE("a deflate block whose output contradicts its header is rejected", "[Binarize][Decompress]") {
    FileMetadataBlock block;
    const EResult result = read_crafted_metadata(ECompressionType::Deflate, /*uncompressed, understated*/ 8,
                                                 static_cast<uint32_t>(kDeflateIni16.size()), kDeflateIni16, block);

    REQUIRE(result != EResult::Success);
}

// Heatshrink sized its output buffer from the declared size up front, so a stream
// that ended early left the tail zero padded and still reported success -
// fabricated bytes the file never contained. A short stream under a large
// declared size must now be rejected rather than padded.
TEST_CASE("a heatshrink block that underfills its declared size is rejected", "[Binarize][Decompress]") {
    const std::vector<uint8_t> payload = {0x80, 0x41, 0x00};

    FileMetadataBlock block;
    const EResult result = read_crafted_metadata(ECompressionType::Heatshrink_11_4, /*uncompressed*/ 512,
                                                 static_cast<uint32_t>(payload.size()), payload, block);

    REQUIRE(result != EResult::Success);
}

// A block header's size fields are attacker-supplied and bore no relation to the
// bytes present, so a tiny file could drive an arbitrary allocation. A declared
// size beyond the configured limit must be refused before anything is allocated
// on it.
TEST_CASE("a block declaring more than the limit is refused", "[Binarize][Decompress]") {
    const ScopedBlockSizeLimit limit(1024);

    // Declared compressed_size well above the limit, with no payload actually
    // present: the pre-fix reader resized to this before reading a single byte.
    FileMetadataBlock block;
    const EResult result = read_crafted_metadata(ECompressionType::Deflate, /*uncompressed*/ 64,
                                                 /*compressed*/ 8 * 1024, std::vector<uint8_t>{}, block);

    REQUIRE(result == EResult::BlockTooLarge);
}
