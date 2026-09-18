#include <e2ee/e2ee.hpp>
#include <core/core.hpp>
#include <unique_file_ptr.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <array>
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

// Positions the file where read_and_verify_identity_block resumes (the
// identity block content), returning its BlockHeader.
bgcode::core::BlockHeader find_identity_block(FILE *file) {
    bgcode::core::FileHeader file_header;
    REQUIRE(bgcode::core::read_header(*file, file_header, nullptr) == bgcode::core::EResult::Success);
    bgcode::core::BlockHeader block_header;
    REQUIRE(bgcode::core::read_next_block_header(*file, file_header, block_header, bgcode::core::EBlockType::IdentityBlock) == bgcode::core::EResult::Success);
    return block_header;
}

struct IdentityBlockFixture {
    const char *filename;
    const char *expected_name;
    bool expected_one_time;
    std::array<uint8_t, e2ee::HASH_SIZE> expected_key_block_hash;
};

// Runs the parser over payload with the given block_size and returns the
// resulting error text (nullptr on success).
const char *parse_identity_block(const std::vector<uint8_t> &payload, uint32_t block_size) {
    unique_file_ptr file = identity_block_file(payload);
    bgcode::core::BlockHeader header = identity_header(block_size);
    e2ee::IdentityBlockInfo info;
    return e2ee::read_and_verify_identity_block(file.get(), header, nullptr, info, false);
}

// Largest block_size the parser accepts. A plain literal on purpose (the
// constant computation is awkward and long). The two boundary tests below
// bracket the real cap, so a drift in either direction fails one of them and
// points here to update.
constexpr uint32_t MAX_LEGAL_BLOCK_SIZE = 754;

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

TEST_CASE("read_and_verify_identity_block rejects an oversized block_size") {
    // A huge attacker-controlled block_size must be rejected before any payload
    // is read. MAX_LEGAL_BLOCK_SIZE + 1 pins the cap's upper edge.
    const uint32_t block_size = GENERATE(MAX_LEGAL_BLOCK_SIZE + 1, 0x10000, 0xFFFFFFFF);
    CAPTURE(block_size);

    std::vector<uint8_t> payload;
    const char *err = parse_identity_block(payload, block_size);
    REQUIRE(err != nullptr);
    CHECK(std::string_view(err) == e2ee::identity_parsing_error);
}

TEST_CASE("read_and_verify_identity_block accepts block_size at the cap") {
    // The largest legal block passes the size cap; with no payload present it
    // then fails on the truncated read rather than the cap, proving the cap is
    // exactly one byte higher than the rejected size above.
    const char *err = parse_identity_block({}, MAX_LEGAL_BLOCK_SIZE);
    REQUIRE(err != nullptr);
    CHECK(std::string_view(err) == e2ee::file_error);
}

// Parses real fixtures produced by the reference tool (utils/e2ee/encrypt_bgcode.py),
// checking compatibility of field extraction against an independent implementation.
// The fixtures are shared with the gcode/reader tests; the golden key_block_hash
// values were extracted from them by that tool, not hand-typed.
TEST_CASE("read_and_verify_identity_block accepts a real identity block") {
    const IdentityBlockFixture fixture = GENERATE(
        IdentityBlockFixture {
            "./box_new_enc.bgcode", "slicer", false,
            { 0x1b, 0x74, 0x24, 0xef, 0x94, 0x25, 0x3f, 0x81,
                0x8d, 0x94, 0xd7, 0xfa, 0x5e, 0x7f, 0xc9, 0x88,
                0x03, 0x50, 0xab, 0x9a, 0xe5, 0x2f, 0xfe, 0x7a,
                0x4e, 0x83, 0x24, 0xe8, 0xbb, 0x59, 0x97, 0xff } },
        IdentityBlockFixture {
            "./box_new_enc_multiple.bgcode", "slicer", false,
            { 0xc8, 0x39, 0x61, 0x8a, 0x9e, 0x34, 0x69, 0x01,
                0xa3, 0xfd, 0x48, 0x26, 0x77, 0x35, 0x88, 0x4c,
                0x18, 0x87, 0xeb, 0xcd, 0xd9, 0x8f, 0x52, 0x05,
                0x75, 0xb4, 0x3c, 0x60, 0x01, 0xb4, 0xa5, 0x3d } });
    CAPTURE(fixture.filename);

    unique_file_ptr file(fopen(fixture.filename, "rb"));
    REQUIRE(file.get() != nullptr);
    const bgcode::core::BlockHeader block_header = find_identity_block(file.get());

    e2ee::IdentityBlockInfo info;
    const char *err = e2ee::read_and_verify_identity_block(file.get(), block_header, nullptr, info, false);
    REQUIRE(err == nullptr);
    CHECK(std::string_view(info.identity_name.data()) == fixture.expected_name);
    CHECK(info.one_time_identity == fixture.expected_one_time);
    CHECK(info.key_block_hash == fixture.expected_key_block_hash);
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
