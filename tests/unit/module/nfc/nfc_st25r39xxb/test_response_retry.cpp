#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "fake_chip.hpp"
#include "tag_response.hpp"

#include <nfcv/commands.hpp>
#include <nfcv/error.hpp>

#include <array>
#include <vector>

using fake::FakeChip;
using fake::FakeSys;
using st25r39xxb::Command;
using st25r39xxb::IRQType;

namespace {

constexpr auto max_attempts = st25r39xxb::ST25R39XXB::nfcv_command_max_attempts;

constexpr std::array<uint8_t, 4> block_payload { 16, 17, 18, 19 };
constexpr std::array<std::byte, 4> expected_block {
    std::byte { 16 }, std::byte { 17 }, std::byte { 18 }, std::byte { 19 }
};

/// A well formed answer to a four byte ReadSingleBlock
std::vector<std::byte> good_response() {
    return fake::encode_tag_response(0, block_payload);
}

/// The same answer with its SOF destroyed. A disturbance anywhere in the frame looks like this to
/// the decoder: bits that are no longer a coding of anything.
std::vector<std::byte> undecodable_response() {
    auto response = good_response();
    response.front() &= std::byte { 0xf0 };
    return response;
}

/// A frame that decodes cleanly but whose CRC belongs to no payload
std::vector<std::byte> bad_crc_response() {
    static constexpr std::array<uint8_t, 7> frame { 0, 16, 17, 18, 19, 0, 0 };
    return fake::encode_tag_frame(frame);
}

/// The same answer cut in half, so the frame never reaches its EOF
std::vector<std::byte> truncated_response() {
    auto response = good_response();
    response.resize(response.size() / 2);
    return response;
}

/// A tag reporting a request it will not serve - a real answer rather than a damaged one
std::vector<std::byte> error_flag_response() {
    return fake::encode_tag_response(1, {});
}

} // namespace

TEST_CASE("the response encoder agrees with the decoder", "[st25r39xxb][response]") {
    // Captured from a real tag, and decoded byte for byte by the nfcv decode tests. The tests below
    // are only worth as much as the answers they put in the FIFO, so pin the encoder against it.
    static constexpr std::array<uint8_t, 16> captured {
        0xb7, 0xaa, 0xaa, 0xca, 0xca, 0xca, 0x2a, 0xcb, 0x4a, 0xcb, 0xaa, 0x2c, 0x53, 0xcd, 0xac, 0x03
    };

    const auto encoded = good_response();
    REQUIRE(encoded.size() == captured.size());
    for (size_t i = 0; i < captured.size(); i++) {
        CAPTURE(i);
        CHECK(encoded[i] == std::byte { captured[i] });
    }
}

TEST_CASE("a damaged answer is asked for again", "[st25r39xxb][response]") {
    FakeChip chip;
    FakeSys sys { chip };
    st25r39xxb::ST25R39XXB reader { chip, sys };

    std::array<std::byte, 4> block {};
    const nfcv::Command command { nfcv::command::ReadSingleBlock { { nfcv::UID {}, 0 }, block } };

    SECTION("an answer that arrives intact costs one exchange") {
        chip.response_script = { good_response() };

        REQUIRE(reader.nfcv_command(command).has_value());
        CHECK(block == expected_block);
        CHECK(chip.command_count(Command::transmit_without_crc) == 1);
    }

    SECTION("an answer damaged once is asked for again and then read") {
        chip.response_script = { GENERATE(undecodable_response(), bad_crc_response(), truncated_response()), good_response() };

        REQUIRE(reader.nfcv_command(command).has_value());
        CHECK(block == expected_block);
        CHECK(chip.command_count(Command::transmit_without_crc) == 2);
    }

    SECTION("an answer damaged every time gives up after the full number of attempts") {
        chip.response_script = { undecodable_response() };

        CHECK(reader.nfcv_command(command) == std::unexpected(nfcv::Error::response_format_invalid));
        CHECK(chip.command_count(Command::transmit_without_crc) == max_attempts);
    }

    SECTION("a tag reporting an error is taken at its word") {
        // The exchange itself went through and the answer is intact - asking again gets the same
        // answer, so the attempts would be spent for nothing.
        chip.response_script = { error_flag_response() };

        CHECK(reader.nfcv_command(command) == std::unexpected(nfcv::Error::response_is_error));
        CHECK(chip.command_count(Command::transmit_without_crc) == 1);
    }

    SECTION("every attempt sends the same request") {
        // The response shares the driver's one buffer with the request, so each attempt has to build
        // the request again rather than transmit whatever the last response left behind.
        chip.response_script = { undecodable_response() };

        CHECK(!reader.nfcv_command(command).has_value());
        REQUIRE(chip.transmitted.size() == max_attempts);
        CHECK(!chip.transmitted.front().empty());
        for (const auto &sent : chip.transmitted) {
            CHECK(sent == chip.transmitted.front());
        }
    }
}

TEST_CASE("every exchange starts from a reset receiver", "[st25r39xxb][response]") {
    // DS13541 §4.2.13 opens every transceive sequence with Stop all activities and Reset RX gain.
    // Doing it once at field-up is not enough: the gain reduction the squelch and AGC loops take
    // during one exchange stays until it is cleared (DS13541 §4.4.9), so the exchange after a
    // strong disturbance would listen to the tag through a receiver still turned down for it.
    FakeChip chip;
    FakeSys sys { chip };
    st25r39xxb::ST25R39XXB reader { chip, sys };

    std::array<std::byte, 4> block {};
    const nfcv::Command command { nfcv::command::ReadSingleBlock { { nfcv::UID {}, 0 }, block } };

    /// The command sequence one attempt is made of, repeated \p attempts times
    const auto transceive_sequence = [](size_t attempts) {
        std::vector<Command> expected;
        for (size_t attempt = 0; attempt < attempts; attempt++) {
            expected.insert(expected.end(), { Command::stop_all_1, Command::reset_rx_gain, Command::transmit_without_crc });
        }
        return expected;
    };

    SECTION("one exchange") {
        chip.response_script = { good_response() };

        REQUIRE(reader.nfcv_command(command).has_value());
        CHECK(chip.commands == transceive_sequence(1));
    }

    SECTION("and every retry of it") {
        // A tag that says nothing, so every attempt is spent
        chip.transmit_script = { IRQType::tx_end };

        CHECK(reader.nfcv_command(command) == std::unexpected(nfcv::Error::no_response));
        CHECK(chip.commands == transceive_sequence(max_attempts));
    }
}

TEST_CASE("a command that expects no answer still completes", "[st25r39xxb][response]") {
    FakeChip chip;
    FakeSys sys { chip };
    st25r39xxb::ST25R39XXB reader { chip, sys };

    // StayQuiet is unanswered by design, so nothing is decoded and nothing is retried
    chip.transmit_script = { IRQType::tx_end };
    const nfcv::Command command { nfcv::command::StayQuiet { { nfcv::UID {} } } };

    REQUIRE(reader.nfcv_command(command).has_value());
    CHECK(chip.command_count(Command::transmit_without_crc) == 1);
    CHECK(chip.command_count(Command::start_general_purpose_timer) == 1);
}
