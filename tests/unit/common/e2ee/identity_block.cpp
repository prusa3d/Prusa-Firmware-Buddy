#include <e2ee/e2ee.hpp>
#include <core/core.hpp>
#include <unique_file_ptr.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cstdio>
#include <cstring>
#include <string_view>
#include <utility>
#include <vector>

namespace {

// Builds a tmpfile containing algo(uint16=RSA) + flags(uint8) + payload, rewound.
unique_file_ptr identity_block_file(const std::vector<uint8_t> &payload) {
    unique_file_ptr file(tmpfile());
    REQUIRE(file.get() != nullptr);

    const uint16_t algo = std::to_underlying(bgcode::core::EIdentityBlockSignCypher::RSA);
    const uint8_t flags = 0;
    REQUIRE(fwrite(&algo, 1, sizeof(algo), file.get()) == sizeof(algo));
    REQUIRE(fwrite(&flags, 1, sizeof(flags), file.get()) == sizeof(flags));
    REQUIRE(fwrite(payload.data(), 1, payload.size(), file.get()) == payload.size());
    REQUIRE(fseek(file.get(), 0, SEEK_SET) == 0);
    return file;
}

bgcode::core::BlockHeader identity_header(uint32_t uncompressed_size) {
    return bgcode::core::BlockHeader(
        std::to_underlying(bgcode::core::EBlockType::IdentityBlock),
        std::to_underlying(bgcode::core::ECompressionType::None),
        uncompressed_size);
}

// Runs the parser over payload with the given block_size and returns the
// resulting error text (nullptr on success).
const char *parse_identity_block(const std::vector<uint8_t> &payload, uint32_t block_size) {
    unique_file_ptr file = identity_block_file(payload);
    bgcode::core::BlockHeader header = identity_header(block_size);
    e2ee::IdentityBlockInfo info;
    return e2ee::read_and_verify_identity_block(file.get(), header, nullptr, info, false);
}

} // namespace

TEST_CASE("read_and_verify_identity_block rejects an undersized block_size") {
    const uint32_t block_size = GENERATE(0, 244, 245, 250, 255, 322);
    CAPTURE(block_size);

    // Large enough that an unenforced size bound would let fread overflow the allocation.
    std::vector<uint8_t> payload(4096, 0x41);
    const char *err = parse_identity_block(payload, block_size);
    REQUIRE(err != nullptr);
    CHECK(std::string_view(err) == e2ee::identity_parsing_error);
}

TEST_CASE("read_and_verify_identity_block rejects an oversized key_len") {
    const uint32_t block_size = 400;
    const size_t payload_size = block_size - e2ee::SIGN_SIZE;

    std::vector<uint8_t> payload(payload_size, 0);
    const uint16_t key_len = 0xFFFF;
    memcpy(payload.data(), &key_len, sizeof(key_len));

    const char *err = parse_identity_block(payload, block_size);
    REQUIRE(err != nullptr);
    CHECK(std::string_view(err) == e2ee::identity_parsing_error);
}

TEST_CASE("read_and_verify_identity_block rejects an oversized name_len") {
    // Minimal legal block_size: key_len=0 leaves exactly name_len(1) + 2*HASH_SIZE(64)
    // bytes, so any name_len >= 1 must be rejected.
    const uint32_t block_size = 323;
    const size_t payload_size = block_size - e2ee::SIGN_SIZE;

    std::vector<uint8_t> payload(payload_size, 0);
    const uint16_t key_len = 0;
    memcpy(payload.data(), &key_len, sizeof(key_len));
    const uint8_t name_len = 1;
    payload[sizeof(key_len)] = name_len;

    const char *err = parse_identity_block(payload, block_size);
    REQUIRE(err != nullptr);
    CHECK(std::string_view(err) == e2ee::identity_parsing_error);
}
