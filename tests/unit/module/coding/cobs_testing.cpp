#include <catch2/catch_test_macros.hpp>
#include <cobs/cobs.hpp>
#include <cstddef>
#include <vector>
#include <algorithm>

namespace {
// Build a byte vector from integer literals, keeping the encoded/decoded
// test data readable.
std::vector<std::byte> b(std::initializer_list<int> values) {
    std::vector<std::byte> out;
    out.reserve(values.size());
    for (int v : values) {
        out.push_back(static_cast<std::byte>(v));
    }
    return out;
}
} // namespace

// Helper to create a valid COBS message of specific decoded length
std::vector<std::byte> create_cobs_message(size_t decoded_length) {
    std::vector<std::byte> encoded;
    size_t remaining = decoded_length;

    while (remaining > 0) {
        size_t block_size = std::min(remaining, size_t { 254 });
        encoded.push_back(static_cast<std::byte>(block_size + 1)); // Code byte
        for (size_t i = 0; i < block_size; i++) {
            encoded.push_back(std::byte { 0x42 }); // Non-zero data
        }
        remaining -= block_size;
    }
    // If the original length was 0, OR if it was a perfect multiple of 254,
    // we must append the final '0x01' block to terminate the data.
    if (decoded_length == 0 || (decoded_length % 254 == 0)) {
        encoded.push_back(std::byte { 0x01 });
    }

    encoded.push_back(cobs::DELIMITER); // Delimiter
    return encoded;
}

namespace cobs {
TEST_CASE("COBS Decoder - invalid buffers", "[cobs][decoder]") {
    constexpr size_t BUFFER_SIZE = 300;

    auto expect_error = [&](Bytes data, CobsError expected_error) {
        ArrayCobsStreamDecoder<BUFFER_SIZE> decoder;

        decoder.add_bytes(
            data,
            [](Bytes) -> void {
                FAIL("Finish callback should not be called");
            },
            [&expected_error, &decoder](CobsError error) -> void {
                REQUIRE(error == expected_error);
                decoder.reset(CobsStreamDecoder::Mode::error);
            });

        REQUIRE(decoder.get_mode() == CobsStreamDecoder::Mode::error);
    };

    SECTION("Invalidly encoded message") {
        auto invalid_data = b({ 0x05, 0x11, 0x22, 0x00 }); // Overhead byte says 5, but only 2 data bytes (invalid)
        expect_error(invalid_data, CobsError::invalid_input);
    }

    SECTION("Send only delimiter (not a valid message)") {
        auto invalid_data = b({ 0x00 }); // Missing code byte (invalid)
        expect_error(invalid_data, CobsError::invalid_input);
    }

    SECTION("Message longer than buffer capacity") {
        std::vector<std::byte> overflow_data = create_cobs_message(BUFFER_SIZE + 1);
        expect_error(overflow_data, CobsError::overflow);
    }

    SECTION("Invalid 0xFF block (terminated by zero)") {
        std::vector<std::byte> invalid_data(256);
        invalid_data[0] = std::byte { 0xFF };
        std::fill(invalid_data.begin() + 1, invalid_data.begin() + 255, std::byte { 0x42 });
        invalid_data[255] = std::byte { 0x00 }; // 0xFF block must be followed by code, not 0x00

        // We only need to feed the part that fits
        expect_error(invalid_data, CobsError::invalid_input);
    }
}

TEST_CASE("COBS Decoder - valid buffers", "[cobs][decoder]") {
    constexpr size_t BUFFER_SIZE = 256;
    std::array<std::byte, BUFFER_SIZE> input_buffer;

    auto expect_decoded = [&](std::vector<std::byte> &input, std::vector<std::byte> &expected) {
        CobsStreamDecoder decoder(input_buffer);
        bool finished = false;

        input.push_back(cobs::DELIMITER);

        decoder.add_bytes(
            input,
            [&expected, &finished](Bytes decoded) -> void {
                REQUIRE(std::equal(decoded.begin(), decoded.end(), expected.begin(), expected.end()));
                finished = true;
            },
            [](CobsError error) -> void {
                FAIL("Error should not be reachable");
            });
        REQUIRE(finished);
    };

    SECTION("Empty message") {
        auto encoded = b({ 0x01 });
        auto expected = b({}); // Empty

        expect_decoded(encoded, expected);
    }

    SECTION("Delimeter (zero) in the middle") {
        auto encoded = b({ 0x03, 0xDE, 0xAD, 0x03, 0xBE, 0xEF });
        auto expected = b({ 0xDE, 0xAD, 0x00, 0xBE, 0xEF });

        expect_decoded(encoded, expected);
    }

    SECTION("Delimeter (zero) at start") {
        auto encoded = b({ 0x01, 0x03, 0x01, 0x02 });
        auto expected = b({ 0x00, 0x01, 0x02 });

        expect_decoded(encoded, expected);
    }

    SECTION("Delimeter (zero) at end") {
        auto encoded = b({ 0x03, 0x01, 0x02, 0x01 });
        auto expected = b({ 0x01, 0x02, 0x00 });

        expect_decoded(encoded, expected);
    }

    SECTION("Multiple consecutive zeros") {
        auto encoded = b({ 0x01, 0x01, 0x01, 0x01 });
        auto expected = b({ 0x00, 0x00, 0x00 });

        expect_decoded(encoded, expected);
    }

    SECTION("Alternating zeros and data") {
        auto encoded = b({ 0x02, 0x01, 0x02, 0x02, 0x02, 0x03 });
        auto expected = b({ 0x01, 0x00, 0x02, 0x00, 0x03 });

        expect_decoded(encoded, expected);
    }

    SECTION("Two zeros separated by data") {
        auto encoded = b({ 0x01, 0x02, 0x11, 0x01 });
        auto expected = b({ 0x00, 0x11, 0x00 });

        expect_decoded(encoded, expected);
    }

    SECTION("10 non-zero bytes") {
        auto encoded = b({ 0x0B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
        std::vector<std::byte> expected(10, std::byte { 0xFF });

        expect_decoded(encoded, expected);
    }

    SECTION("Min/max byte pattern with zeros") {
        auto encoded = b({ 0x01, 0x02, 0xFF, 0x01 });
        auto expected = b({ 0x00, 0xFF, 0x00 });

        expect_decoded(encoded, expected);
    }

    SECTION("Pattern: 0x00 0x00 0xFF") {
        auto encoded = b({ 0x01, 0x01, 0x02, 0xFF });
        auto expected = b({ 0x00, 0x00, 0xFF });

        expect_decoded(encoded, expected);
    }

    /*
        Following rule is commonly enforced in COBS decoders:
        * A 0x00 after 0xFF is invalid.
        * A 0x00 at the start is invalid unless it’s a delimiter.

        You shouldn’t see 0x00 where a code byte is expected (except as a delimiter between packets).

        Python "cobs" lib does not enforce this...
    */
    SECTION("254 non-zero bytes") {
        // Encoded: { 0xFF, [254 bytes], 0x01}
        std::vector<std::byte> expected(254, std::byte { 0x42 });
        std::vector<std::byte> encoded;
        encoded.push_back(std::byte { 0xFF });
        encoded.insert(encoded.end(), expected.begin(), expected.end());
        encoded.push_back(std::byte { 0x01 });

        expect_decoded(encoded, expected);
    }

    SECTION("255 non-zero bytes") {
        // Encoded: { 0xFF, [254 bytes], 0x02, 0x42}
        std::vector<std::byte> expected(255, std::byte { 0x42 });
        std::vector<std::byte> encoded;
        encoded.push_back(std::byte { 0xFF });
        encoded.insert(encoded.end(), expected.begin(), expected.begin() + 254);
        encoded.push_back(std::byte { 0x02 });
        encoded.push_back(std::byte { 0x42 });

        expect_decoded(encoded, expected);
    }
}

TEST_CASE("COBS Decoder - Streaming (Partial Messages)", "[cobs][decoder]") {
    ArrayCobsStreamDecoder<64> decoder;
    bool finished = false;

    auto expected = b({ 0xDE, 0xAD, 0x00, 0xBE, 0xEF });

    auto finish_cb = [&](Bytes decoded) -> void {
        REQUIRE(std::equal(decoded.begin(), decoded.end(), expected.begin(), expected.end()));
        finished = true;
    };

    auto error_cb = [](CobsError error) -> void {
        FAIL("Error should not be reachable");
    };

    SECTION("Message split in two") {
        // Encoded: { 0x03, 0xDE, 0xAD, 0x03, 0xBE, 0xEF, 0x00 }
        auto chunk1 = b({ 0x03, 0xDE, 0xAD });
        auto chunk2 = b({ 0x03, 0xBE, 0xEF, 0x00 });

        decoder.add_bytes(chunk1, finish_cb, error_cb);
        REQUIRE_FALSE(finished); // Message is not complete yet

        decoder.add_bytes(chunk2, finish_cb, error_cb);
        REQUIRE(finished); // Now it's complete
    }

    SECTION("Message fed byte-by-byte") {
        auto encoded_message = b({ 0x03, 0xDE, 0xAD, 0x03, 0xBE, 0xEF, 0x00 });

        for (size_t i = 0; i < encoded_message.size() - 1; ++i) {
            std::array<std::byte, 1> byte_chunk = { encoded_message[i] };
            decoder.add_bytes(byte_chunk, finish_cb, error_cb);
            REQUIRE_FALSE(finished); // Not complete yet
        }

        // Feed the final delimiter
        std::array<std::byte, 1> final_byte = { encoded_message.back() };
        decoder.add_bytes(final_byte, finish_cb, error_cb);
        REQUIRE(finished); // Complete
    }
}

TEST_CASE("COBS Decoder - Reset", "[cobs][decoder]") {
    ArrayCobsStreamDecoder<64> decoder;
    bool finished = false;

    auto expected = b({ 0xAA, 0xBB }); // The second message

    auto finish_cb = [&](Bytes decoded) -> void {
        REQUIRE(std::equal(decoded.begin(), decoded.end(), expected.begin(), expected.end()));
        finished = true;
    };
    auto error_cb = [](CobsError error) -> void {
        FAIL("Error should not be reachable");
    };

    // 1. Feed a partial (invalid) message
    auto partial_msg = b({ 0x03, 0xDE, 0xAD });
    decoder.add_bytes(partial_msg, finish_cb, error_cb);
    REQUIRE_FALSE(finished);

    // 2. Reset the decoder
    decoder.reset();
    REQUIRE_FALSE(finished);

    // 3. Feed a new, complete message
    auto new_msg = b({ 0x03, 0xAA, 0xBB, 0x00 });
    decoder.add_bytes(new_msg, finish_cb, error_cb);

    // 4. Check that only the *second* message was decoded
    REQUIRE(finished);
}

TEST_CASE("COBS Decoder -Two messages in a buffer", "[cobs][decoder]") {
    constexpr size_t BUFFER_SIZE = 20;
    std::array<std::byte, BUFFER_SIZE> input_buffer;

    auto expect_multiple_same_decoded = [&](std::vector<std::byte> &input, std::vector<std::byte> &expected, uint8_t cnt) {
        CobsStreamDecoder decoder(input_buffer);
        uint8_t finished_cnt = 0;

        decoder.add_bytes(
            input,
            [&finished_cnt, &expected](Bytes decoded) -> void {
                REQUIRE(std::equal(decoded.begin(), decoded.end(), expected.begin(), expected.end()));
                finished_cnt += 1;
            },
            [](CobsError error) -> void {
                FAIL("Error should not be reachable");
            });
        REQUIRE(finished_cnt == cnt);
    };

    SECTION("Multiple same messages in one buffer") {
        auto multiple_messages_encoded = b({ 0x01, 0x01, 0x02, 0xFF, 0x00, 0x01, 0x01, 0x02, 0xFF, 0x00, 0x01, 0x01, 0x02, 0xFF, 0x00, 0x01, 0x01, 0x02, 0xFF, 0x00 });
        auto expected = b({ 0x00, 0x00, 0xFF });

        expect_multiple_same_decoded(multiple_messages_encoded, expected, 4);
    }
}
TEST_CASE("COBS Encoder - static encode function", "[cobs][encoder]") {

    SECTION("Typical usage") {

        auto decoded = b({ 0xDE, 0xAD, 0x00, 0xBE, 0xEF });
        auto expected = b({ 0x03, 0xDE, 0xAD, 0x03, 0xBE, 0xEF, 0x00 });
        std::vector<std::byte> output(expected.size());

        auto ret = encode(decoded, output);
        REQUIRE(ret.has_value());
        REQUIRE(expected == output);
    }

    SECTION("Empty message") {

        auto expected = b({ 0x01, 0x00 });
        std::vector<std::byte> output(expected.size());

        auto ret = encode({}, output);
        REQUIRE(ret.has_value());
        REQUIRE(expected == output);
    }

    SECTION("Overflow due to empty output buffer") {

        auto ret = encode({}, {});
        REQUIRE(!ret.has_value());
        REQUIRE(ret == std::unexpected(CobsError::overflow));
    }

    SECTION("Overflow due to small output buffer") {
        auto decoded = b({ 0xDE, 0xAD, 0x00, 0xBE, 0xEF });
        auto expected = b({ 0x03, 0xDE, 0xAD, 0x03, 0xBE, 0xEF, 0x00 });
        std::vector<std::byte> output(expected.size() - 1);

        auto ret = encode(decoded, output);
        REQUIRE(!ret.has_value());
    }
}

TEST_CASE("COBS Encoder - invalid buffers", "[cobs][encoder]") {

    SECTION("Input buffer overflow (add_bytes)") {
        ArrayCobsStreamEncoder<5> encoder;

        auto data1 = b({ 1, 2, 3 });
        auto result1 = encoder.add_bytes(data1);
        REQUIRE(result1.has_value());

        auto data2 = b({ 4, 5, 6 }); // This will overflow
        auto result2 = encoder.add_bytes(data2);

        REQUIRE_FALSE(result2.has_value());
        REQUIRE(result2.error() == CobsError::overflow);
    }

    SECTION("Encoded buffer overflow (finalize)") {
        // We must use the base class to force this error, since
        // ArrayCobsStreamEncoder *always* creates a validly-sized encoded buffer.

        std::array<std::byte, 300> input_buffer; // Plenty of input space
        std::array<std::byte, 255> encoded_buffer; // *Too small* for the encoded output

        CobsStreamEncoder encoder(input_buffer, encoded_buffer);

        // Input: 254 non-zero bytes.
        // Encoded: { 0xFF, [254 bytes], 0x01 } (256 bytes)
        std::vector<std::byte> decoded(300, std::byte { 0x42 });

        auto add_result = encoder.add_bytes(decoded);
        REQUIRE(add_result.has_value()); // Input add is fine

        // Finalize should fail, needs 256 bytes but only has 255
        auto finalize_result = encoder.finalize();
        REQUIRE_FALSE(finalize_result.has_value());
        REQUIRE(finalize_result.error() == CobsError::overflow);
    }
}

TEST_CASE("COBS Encoder - valid buffers", "[cobs][encoder]") {
    constexpr size_t BUFFER_SIZE = 64;
    ArrayCobsStreamEncoder<BUFFER_SIZE> encoder;
    REQUIRE(max_encoded_frame_size(BUFFER_SIZE) == 66);

    auto expect_encoded = [&](std::vector<std::byte> &decoded,
                              std::vector<std::byte> &expected_encoded) {
        encoder.reset();

        auto add_result = encoder.add_bytes(decoded);
        REQUIRE(add_result.has_value());

        auto finalize_result = encoder.finalize();
        REQUIRE(finalize_result.has_value());

        Bytes actual_output = finalize_result.value();

        // Check correct length
        REQUIRE(actual_output.size() == expected_encoded.size());

        // Check correct content
        REQUIRE(std::equal(actual_output.begin(), actual_output.end(),
            expected_encoded.begin(), expected_encoded.end()));
    };

    SECTION("Empty message") {
        auto decoded = b({});
        auto expected = b({ 0x01, 0x00 });
        expect_encoded(decoded, expected);
    }

    SECTION("Delimiter (zero) in the middle") {
        auto decoded = b({ 0xDE, 0xAD, 0x00, 0xBE, 0xEF });
        auto expected = b({ 0x03, 0xDE, 0xAD, 0x03, 0xBE, 0xEF, 0x00 });
        expect_encoded(decoded, expected);
    }

    SECTION("Delimiter (zero) at start") {
        auto decoded = b({ 0x00, 0x01, 0x02 });
        auto expected = b({ 0x01, 0x03, 0x01, 0x02, 0x00 });
        expect_encoded(decoded, expected);
    }

    SECTION("Delimiter (zero) at end") {
        auto decoded = b({ 0x01, 0x02, 0x00 });
        auto expected = b({ 0x03, 0x01, 0x02, 0x01, 0x00 });
        expect_encoded(decoded, expected);
    }

    SECTION("Multiple consecutive zeros") {
        auto decoded = b({ 0x00, 0x00, 0x00 });
        auto expected = b({ 0x01, 0x01, 0x01, 0x01, 0x00 });
        expect_encoded(decoded, expected);
    }

    SECTION("Alternating zeros and data") {
        auto decoded = b({ 0x01, 0x00, 0x02, 0x00, 0x03 });
        auto expected = b({ 0x02, 0x01, 0x02, 0x02, 0x02, 0x03, 0x00 });
        expect_encoded(decoded, expected);
    }

    SECTION("Two zeros separated by data") {
        auto decoded = b({ 0x00, 0x11, 0x00 });
        auto expected = b({ 0x01, 0x02, 0x11, 0x01, 0x00 });
        expect_encoded(decoded, expected);
    }

    SECTION("10 non-zero bytes") {
        std::vector<std::byte> decoded(10, std::byte { 0xFF });
        auto expected = b({ 0x0B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 });
        expect_encoded(decoded, expected);
    }

    SECTION("Min/max byte pattern with zeros") {
        auto decoded = b({ 0x00, 0xFF, 0x00 });
        auto expected = b({ 0x01, 0x02, 0xFF, 0x01, 0x00 });
        expect_encoded(decoded, expected);
    }

    SECTION("Pattern: 0x00 0x00 0xFF") {
        auto decoded = b({ 0x00, 0x00, 0xFF });
        auto expected = b({ 0x01, 0x01, 0x02, 0xFF, 0x00 });
        expect_encoded(decoded, expected);
    }
}

TEST_CASE("COBS Encoder - long messages", "[cobs][encoder]") {
    ArrayCobsStreamEncoder<300> encoder; // Big enough for these

    auto expect_encoded = [&](std::vector<std::byte> &decoded,
                              std::vector<std::byte> &expected_encoded) {
        encoder.reset();
        REQUIRE(encoder.add_bytes(decoded).has_value());
        auto finalize_result = encoder.finalize();
        REQUIRE(finalize_result.has_value());

        Bytes actual_output = finalize_result.value();
        REQUIRE(actual_output.size() == expected_encoded.size());
        REQUIRE(std::equal(actual_output.begin(), actual_output.end(),
            expected_encoded.begin(), expected_encoded.end()));
    };

    /*
    Following rule is commonly enforced in COBS decoders:
        * A 0x00 after 0xFF is invalid.
        * A 0x00 at the start is invalid unless it’s a delimiter.

        You shouldn’t see 0x00 where a code byte is expected (except as a delimiter between packets).

        Python "cobs" lib does not enforce this...
    */
    SECTION("254 non-zero bytes") {
        std::vector<std::byte> decoded(254, std::byte { 0x42 });

        std::vector<std::byte> expected;
        expected.push_back(std::byte { 0xFF }); // Code for 254 bytes
        expected.insert(expected.end(), decoded.begin(), decoded.end());
        expected.push_back(std::byte { 0x01 }); // Code for end of message
        expected.push_back(cobs::DELIMITER); // Code for end of message

        expect_encoded(decoded, expected);
    }

    SECTION("255 non-zero bytes") {
        std::vector<std::byte> decoded(255, std::byte { 0x42 });

        std::vector<std::byte> expected;
        expected.push_back(std::byte { 0xFF }); // Code for first 254 bytes
        expected.insert(expected.end(), decoded.begin(), decoded.begin() + 254);
        expected.push_back(std::byte { 0x02 }); // Code for last 1 byte
        expected.push_back(decoded.back()); // Last byte
        expected.push_back(cobs::DELIMITER); // Code for end of message

        expect_encoded(decoded, expected);
    }

    SECTION("253 non-zero bytes followed by zero") {
        std::vector<std::byte> decoded(253, std::byte { 0x42 });
        decoded.push_back(std::byte { 0x00 });

        std::vector<std::byte> expected;
        expected.push_back(std::byte { 0xFE }); // Code for 253 bytes
        expected.insert(expected.end(), decoded.begin(), decoded.begin() + 253);
        expected.push_back(std::byte { 0x01 }); // Code for zero
        expected.push_back(cobs::DELIMITER); // Code for end of message

        expect_encoded(decoded, expected);
    }
}

TEST_CASE("COBS Encoder - streaming and reset", "[cobs][encoder]") {
    ArrayCobsStreamEncoder<64> encoder;

    SECTION("Add bytes in multiple calls") {
        auto data1 = b({ 0xDE, 0xAD });
        auto data2 = b({ 0x00, 0xBE, 0xEF });
        auto expected = b({ 0x03, 0xDE, 0xAD, 0x03, 0xBE, 0xEF, 0x00 });

        REQUIRE(encoder.add_bytes(data1).has_value());
        REQUIRE(encoder.add_bytes(data2).has_value());

        auto finalize_result = encoder.finalize();
        REQUIRE(finalize_result.has_value());

        Bytes actual = finalize_result.value();
        REQUIRE(std::equal(actual.begin(), actual.end(), expected.begin(), expected.end()));
    }

    SECTION("Reset clears the encoder") {
        auto data1 = b({ 0x01, 0x02, 0x03 });
        auto expected1 = b({ 0x04, 0x01, 0x02, 0x03, 0x00 });

        // First message
        encoder.add_bytes(data1);
        auto res1 = encoder.finalize();
        REQUIRE(res1.has_value());
        Bytes actual1 = res1.value();
        REQUIRE(std::equal(actual1.begin(), actual1.end(), expected1.begin(), expected1.end()));

        // Reset
        encoder.reset();

        // Second, different message
        auto data2 = b({ 0x00, 0xFF });
        auto expected2 = b({ 0x01, 0x02, 0xFF, 0x00 });

        encoder.add_bytes(data2);
        auto res2 = encoder.finalize();
        REQUIRE(res2.has_value());
        Bytes actual2 = res2.value();
        REQUIRE(std::equal(actual2.begin(), actual2.end(), expected2.begin(), expected2.end()));
    }
}
} // namespace cobs
