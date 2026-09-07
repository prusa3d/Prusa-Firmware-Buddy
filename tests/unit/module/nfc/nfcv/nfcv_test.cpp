#include <catch2/catch_test_macros.hpp>
#include <generators/random_bytes.hpp>

#include <nfcv/decode.hpp>
#include <nfcv/encode.hpp>
#include <utils/byte_utils.hpp>

void test_decode(Bytes input, Bytes expected_output) {
    std::vector<std::byte> output;
    output.resize(input.size());
    std::span output_span { output.data(), output.size() };
    auto res = nfcv::decode(input, output_span);
    REQUIRE(res.has_value());
    REQUIRE(res.value().size() == expected_output.size());

    std::ranges::for_each(std::views::zip(*res, expected_output), [](const auto &val) {
        const auto &[lhs, rhs] = val;
        CAPTURE(lhs, rhs);
        CHECK(lhs == rhs);
    });
}

TEST_CASE("Test NFC-V decode Inventory response", "[nfcv][decode]") {
    static constexpr std::array<uint8_t, 12> expected_output = { 0, 0, 19, 123, 90, 4, 9, 1, 4, 224, 176, 178 };
    std::array<uint8_t, 26> input = { 0xb7, 0xaa, 0xaa, 0xaa, 0x4a, 0xcb, 0x4a, 0x53, 0x2d, 0xd3, 0xac, 0xac, 0xca, 0xb2, 0xca, 0xaa, 0xaa, 0xac, 0xaa, 0x2a, 0xb5, 0x4a, 0x33, 0x4b, 0xb3, 0x03 };
    test_decode(std::as_bytes(std::span { input }), std::as_bytes(std::span { expected_output }));
}

TEST_CASE("Test NFC-V decode SysInfo response - SLIX 2", "[nfcv][decode]") {
    static constexpr std::array<uint8_t, 17> expected_output = { 0, 15, 19, 123, 90, 4, 9, 1, 4, 224, 0, 0, 79, 3, 1, 173, 33 };
    std::array<uint8_t, 36> input = { 0xb7, 0xaa, 0x4a, 0xb5, 0x4a, 0xcb, 0x4a, 0x53, 0x2d, 0xd3, 0xac, 0xac, 0xca, 0xb2, 0xca, 0xaa, 0xaa, 0xac, 0xaa, 0x2a, 0xb5, 0xaa, 0xaa, 0xaa, 0x4a, 0xb5, 0x4c, 0xab, 0xca, 0xaa, 0xca, 0x34, 0xd3, 0x2a, 0xab, 0x03 };
    test_decode(std::as_bytes(std::span { input }), std::as_bytes(std::span { expected_output }));
}

TEST_CASE("Test NFC-V decode SysInfo response - SLIX", "[nfcv][decode]") {
    static constexpr std::array<uint8_t, 17> expected_output = { 0, 15, 110, 118, 241, 108, 80, 1, 4, 224, 0, 0, 27, 3, 1, 31, 116 };
    std::array<uint8_t, 36> input = { 0xb7, 0xaa, 0x4a, 0xb5, 0x2a, 0x35, 0x2d, 0x4d, 0xcd, 0x4a, 0xb5, 0x34, 0xad, 0xca, 0xcc, 0xaa, 0xaa, 0xac, 0xaa, 0x2a, 0xb5, 0xaa, 0xaa, 0xaa, 0x4a, 0xd3, 0x4a, 0xab, 0xca, 0xaa, 0x4a, 0xd5, 0xaa, 0x4c, 0xad, 0x03 };
    test_decode(std::as_bytes(std::span { input }), std::as_bytes(std::span { expected_output }));
}

TEST_CASE("Test NFC-V decode ReadSingleBlock response", "[nfcv][decode]") {
    static constexpr std::array<uint8_t, 7> expected_output = { 0, 16, 17, 18, 19, 164, 87 };
    std::array<uint8_t, 16> input = { 0xb7, 0xaa, 0xaa, 0xca, 0xca, 0xca, 0x2a, 0xcb, 0x4a, 0xcb, 0xaa, 0x2c, 0x53, 0xcd, 0xac, 0x03 };
    test_decode(std::as_bytes(std::span { input }), std::as_bytes(std::span { expected_output }));
}

TEST_CASE("Test NFC-V decode WriteSingleBlock response", "[nfcv][decode]") {
    static constexpr std::array<uint8_t, 3> expected_output = { 0, 120, 240 };
    std::array<uint8_t, 8> input = { 0xb7, 0xaa, 0xaa, 0x52, 0xad, 0x4a, 0xb5, 0x03 };
    test_decode(std::as_bytes(std::span { input }), std::as_bytes(std::span { expected_output }));
}

TEST_CASE("Test NFC-V decode - single buffer decoding", "[nfcv][decode]") {
    static constexpr std::array<uint8_t, 7> expected_output = { 0, 16, 17, 18, 19, 164, 87 };
    std::array<uint8_t, 16> input = { 0xb7, 0xaa, 0xaa, 0xca, 0xca, 0xca, 0x2a, 0xcb, 0x4a, 0xcb, 0xaa, 0x2c, 0x53, 0xcd, 0xac, 0x03 };

    std::span input_span { reinterpret_cast<std::byte *>(input.data()), input.size() };
    auto res = nfcv::decode(std::as_bytes(std::span { input }), input_span);
    if (res.has_value()) {
        CAPTURE(res.value());
    } else {
        CAPTURE(res.error());
    }
    REQUIRE(res.has_value());
    REQUIRE(res.value().size() == expected_output.size());

    std::ranges::for_each(std::views::zip(*res, expected_output), [](const auto &val) {
        const auto &[lhs, rhs] = val;
        CAPTURE(lhs, rhs);
        CHECK(lhs == std::byte { rhs });
    });
}

TEST_CASE("Test NFC-V response deserialization - Inventory response", "[nfcv][deserialization]") {
    static constexpr nfcv::UID expected_uid = { std::byte { 19 }, std::byte { 123 }, std::byte { 90 }, std::byte { 4 }, std::byte { 9 }, std::byte { 1 }, std::byte { 4 }, std::byte { 224 } };
    const std::array<uint8_t, 12> input = { 0, 0, 19, 123, 90, 4, 9, 1, 4, 224, 176, 178 };
    nfcv::UID uid;
    nfcv::Command cmd { nfcv::command::Inventory { .request = {}, .response = uid } };
    const auto res = nfcv::parse_response(std::as_bytes(std::span { input }), cmd);
    REQUIRE(res.has_value());
    REQUIRE(std::ranges::equal(uid, expected_uid));
}

TEST_CASE("Test NFC-V response deserialization - SysInfo response (SLIX 2)", "[nfcv][deserialization]") {
    static constexpr nfcv::UID uid {};
    const std::array<uint8_t, 17> input = { 0, 15, 19, 123, 90, 4, 9, 1, 4, 224, 0, 0, 79, 3, 1, 173, 33 };
    nfcv::TagInfo tag_info;
    nfcv::Command cmd { nfcv::command::SystemInfo { .request = { .uid = uid }, .response = tag_info } };
    const auto res = nfcv::parse_response(std::as_bytes(std::span { input }), cmd);
    REQUIRE(res.has_value());
    CHECK(tag_info.dsfid == 0);
    CHECK(tag_info.afi == 0);
    REQUIRE(tag_info.mem_size.has_value());
    CHECK(tag_info.mem_size->block_size == 4);
    CHECK(tag_info.mem_size->block_count == 80);
    REQUIRE(tag_info.ic_ref.has_value());
    CHECK(tag_info.ic_ref.value() == 0x01);
}

TEST_CASE("Test NFC-V response deserialization - ReadSingleBlock", "[nfcv][deserialization]") {
    static constexpr nfcv::UID uid {};
    const std::array<uint8_t, 7> input = { 0, 16, 17, 18, 19, 164, 87 };

    std::array<std::byte, 4> block_data {};
    static constexpr std::array<std::byte, 4> expected_block_data = { std::byte { 16 }, std::byte { 17 }, std::byte { 18 }, std::byte { 19 } };
    nfcv::Command cmd { nfcv::command::ReadSingleBlock { .request = { .uid = uid, .block_address = 12 }, .response { block_data } } };
    const auto res = nfcv::parse_response(std::as_bytes(std::span { input }), cmd);
    REQUIRE(res.has_value());
    REQUIRE(block_data == expected_block_data);
}

TEST_CASE("Test NFC-V response deserialization - WriteSingleBlock", "[nfcv][deserialization]") {
    static constexpr nfcv::UID uid {};
    static constexpr std::array<std::byte, 4> block_data = { std::byte { 16 }, std::byte { 17 }, std::byte { 18 }, std::byte { 19 } };
    const std::array<uint8_t, 3> input = { 0, 120, 240 };

    nfcv::Command cmd { nfcv::command::WriteSingleBlock { .request = { .uid = uid, .block_address = 12, .block_buffer = block_data } } };
    const auto res = nfcv::parse_response(std::as_bytes(std::span { input }), cmd);
    REQUIRE(res.has_value());
}

TEST_CASE("Test NFC-V response deserialization - expected error small buffer", "[nfcv][deserialization]") {
    const std::array<uint8_t, 2> input = { 0, 0 };
    nfcv::UID uid;
    nfcv::Command cmd { nfcv::command::Inventory { .request = {}, .response = uid } };
    const auto res = nfcv::parse_response(std::as_bytes(std::span { input }), cmd);
    REQUIRE(!res.has_value());
    REQUIRE(res.error() == nfcv::Error::response_invalid_size);
}

TEST_CASE("Test NFC-V response deserialization - expected error response is error", "[nfcv][deserialization]") {
    static constexpr nfcv::UID uid {};
    const std::array<uint8_t, 4> input = { 1, 0, 0x9f, 0x16 };
    nfcv::Command cmd { nfcv::command::ReadSingleBlock { .request = { .uid = uid, .block_address = 12 }, .response = {} } };
    const auto res = nfcv::parse_response(std::as_bytes(std::span { input }), cmd);
    REQUIRE(!res.has_value());
    REQUIRE(res.error() == nfcv::Error::response_is_error);
}

void test_encode(const nfcv::Command &command, Bytes expected_output) {
    stdext::inplace_vector<std::byte, 512> msg_builder;

    const auto res = nfcv::construct_command(msg_builder, command);
    REQUIRE(res.has_value());
    CHECK(expected_output.size() == msg_builder.size());
    for (size_t i = 0; i < std::min(expected_output.size(), msg_builder.size()); ++i) {
        CAPTURE(i, expected_output[i], msg_builder[i]);
        CHECK(expected_output[i] == msg_builder[i]);
    }
}

TEST_CASE("Test NFC-V request encoding - Inventory command", "[nfcv][encode]") {
    nfcv::UID uid {};
    nfcv::Command command { nfcv::command::Inventory { .request = {}, .response = uid } };
    static constexpr std::array<uint8_t, 22> expected_output { 0x21, 0x20, 0x08, 0x20, 0x02, 0x08, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x08, 0x80, 0x80, 0x20, 0x20, 0x02, 0x02, 0x04 };
    test_encode(command, std::as_bytes(std::span { expected_output }));
}

TEST_CASE("Test NFC-V request encoding - SystemInfo command", "[nfcv][encode]") {
    static constexpr nfcv::UID uid = { std::byte { 19 }, std::byte { 123 }, std::byte { 90 }, std::byte { 4 }, std::byte { 9 }, std::byte { 1 }, std::byte { 4 }, std::byte { 224 } };
    nfcv::TagInfo tag_info;
    nfcv::Command command { nfcv::command::SystemInfo { .request = { .uid = uid }, .response = tag_info } };
    static constexpr std::array<uint8_t, 50> expected_output { 0x21, 0x20, 0x2, 0x20, 0x2, 0x80, 0x20, 0x20, 0x2, 0x80, 0x2, 0x8, 0x2, 0x80, 0x20, 0x80, 0x8, 0x20, 0x20, 0x8, 0x8, 0x2, 0x8, 0x2, 0x2, 0x8, 0x20, 0x2, 0x2, 0x8, 0x2, 0x2, 0x2, 0x2, 0x8, 0x2, 0x2, 0x2, 0x2, 0x20, 0x80, 0x80, 0x80, 0x80, 0x20, 0x2, 0x20, 0x8, 0x2, 0x4 };

    test_encode(command, std::as_bytes(std::span { expected_output }));
}

TEST_CASE("Test NFC-V request encoding - ReadSingleBlock command", "[nfcv][encode]") {
    static constexpr nfcv::UID uid = { std::byte { 19 }, std::byte { 123 }, std::byte { 90 }, std::byte { 4 }, std::byte { 9 }, std::byte { 1 }, std::byte { 4 }, std::byte { 224 } };
    std::array<std::byte, 4> block_data {};
    nfcv::Command command { nfcv::command::ReadSingleBlock { .request = { .uid = uid, .block_address = 12 }, .response = block_data } };
    static constexpr std::array<uint8_t, 54> expected_output { 0x21, 0x20, 0x2, 0x20, 0x2, 0x2, 0x2, 0x20, 0x2, 0x80, 0x2, 0x8, 0x2, 0x80, 0x20, 0x80, 0x8, 0x20, 0x20, 0x8, 0x8, 0x2, 0x8, 0x2, 0x2, 0x8, 0x20, 0x2, 0x2, 0x8, 0x2, 0x2, 0x2, 0x2, 0x8, 0x2, 0x2, 0x2, 0x2, 0x20, 0x80, 0x2, 0x80, 0x2, 0x2, 0x2, 0x2, 0x80, 0x2, 0x20, 0x20, 0x20, 0x8, 0x4 };

    test_encode(command, std::as_bytes(std::span { expected_output }));
}

TEST_CASE("Test NFC-V request encoding - WriteSingleBlock command", "[nfcv][encode]") {
    static constexpr nfcv::UID uid = { std::byte { 19 }, std::byte { 123 }, std::byte { 90 }, std::byte { 4 }, std::byte { 9 }, std::byte { 1 }, std::byte { 4 }, std::byte { 224 } };
    std::array<std::byte, 4> block_data {};
    nfcv::Command command { nfcv::command::WriteSingleBlock { .request = { .uid = uid, .block_address = 12, .block_buffer = block_data }, .response = {} } };
    static constexpr std::array<uint8_t, 70> expected_output { 0x21, 0x20, 0x2, 0x20, 0x2, 0x8, 0x2, 0x20, 0x2, 0x80, 0x2, 0x8, 0x2, 0x80, 0x20, 0x80, 0x8, 0x20, 0x20, 0x8, 0x8, 0x2, 0x8, 0x2, 0x2, 0x8, 0x20, 0x2, 0x2, 0x8, 0x2, 0x2, 0x2, 0x2, 0x8, 0x2, 0x2, 0x2, 0x2, 0x20, 0x80, 0x2, 0x80, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x8, 0x2, 0x80, 0x80, 0x2, 0x20, 0x80, 0x20, 0x4 };

    test_encode(command, std::as_bytes(std::span { expected_output }));
}

TEST_CASE("Test NFC-V request encoding - StayQuiet command", "[nfcv][encode]") {
    static constexpr nfcv::UID uid = { std::byte { 19 }, std::byte { 123 }, std::byte { 90 }, std::byte { 4 }, std::byte { 9 }, std::byte { 1 }, std::byte { 4 }, std::byte { 224 } };
    nfcv::Command command { nfcv::command::StayQuiet { .request = { .uid = uid } } };
    static constexpr std::array<uint8_t, 50> expected_output { 0x21, 0x20, 0x2, 0x20, 0x2, 0x20, 0x2, 0x2, 0x2, 0x80, 0x2, 0x8, 0x2, 0x80, 0x20, 0x80, 0x8, 0x20, 0x20, 0x8, 0x8, 0x2, 0x8, 0x2, 0x2, 0x8, 0x20, 0x2, 0x2, 0x8, 0x2, 0x2, 0x2, 0x2, 0x8, 0x2, 0x2, 0x2, 0x2, 0x20, 0x80, 0x8, 0x2, 0x80, 0x20, 0x8, 0x80, 0x8, 0x80, 0x4 };

    test_encode(command, std::as_bytes(std::span { expected_output }));
}
