#include <catch2/catch_test_macros.hpp>

#include "fake_chip.hpp"
#include "tag_response.hpp"

#include <nfcv/commands.hpp>
#include <nfcv/error.hpp>

#include <array>
#include <numeric>
#include <vector>

using fake::FakeChip;
using fake::FakeSys;
using st25r39xxb::Command;
using st25r39xxb::IRQType;

namespace {

/// Drives a read command through the driver, whose await sequence is tx_end, rx_start, rx_end.
/// Unless the chip has been given a response to answer with, the command ends in failure; what the
/// tests here care about is how many transmissions it took to get there.
nfcv::Result<void> read_a_block(st25r39xxb::ST25R39XXB &reader) {
    std::array<std::byte, 4> block {};
    const nfcv::UID uid {};
    const nfcv::Command command { nfcv::command::ReadSingleBlock { { uid, 0 }, block } };
    return reader.nfcv_command(command);
}

/// A well formed answer to the read \ref read_a_block issues
std::vector<std::byte> good_block_response() {
    static constexpr std::array<uint8_t, 4> payload { 16, 17, 18, 19 };
    return fake::encode_tag_response(0, payload);
}

} // namespace

TEST_CASE("repeated arbitration rounds keep working", "[st25r39xxb][interrupts]") {
    FakeChip chip;
    FakeSys sys { chip };
    st25r39xxb::ST25R39XXB reader { chip, sys };

    // Every round toggles the interrupt mask twice and reads the status registers once more than it
    // waits, so an assertion-per-wait accounting drifts further out with each one. The rounds come
    // from whoever is re-arbitrating - nfcv::FieldManager on hardware.
    chip.field_on_script = { IRQType::collision_detected };
    for (int round = 0; round < 4; round++) {
        CHECK(reader.try_field_up(0) == std::unexpected(nfcv::Error::field_collision));
    }
    CHECK(chip.field_on_count == 4);

    // Still healthy afterwards
    chip.field_on_script = { IRQType::pwp_field_active };
    CHECK(reader.try_field_up(0).has_value());
}

TEST_CASE("the asserted line is enough on its own", "[st25r39xxb][interrupts]") {
    FakeChip chip;
    FakeSys sys { chip };
    st25r39xxb::ST25R39XXB reader { chip, sys };

    // The line is a level while the host input is an edge, so an assertion the host never reports -
    // already high when the input was armed, or an edge spent by an earlier await - must not turn
    // into a wait that sleeps through data that is already there.
    sys.deliver_wakeups = false;

    SECTION("field on completes without waiting for a wakeup that will not come") {
        chip.field_on_script = { IRQType::pwp_field_active };
        const auto start = sys.now_ms();
        CHECK(reader.try_field_up(0).has_value());

        // Every millisecond that passed was a deliberate sleep. Anything on top of that would be a
        // wait for an assertion that had already happened.
        CHECK(sys.now_ms() - start == std::accumulate(sys.delays.begin(), sys.delays.end(), uint32_t { 0 }));
    }

    SECTION("a collision is still reported as a collision") {
        chip.field_on_script = { IRQType::collision_detected };
        const auto start = sys.now_ms();
        CHECK(reader.try_field_up(0) == std::unexpected(nfcv::Error::field_collision));
        CHECK(sys.now_ms() - start == std::accumulate(sys.delays.begin(), sys.delays.end(), uint32_t { 0 }));
    }

    SECTION("a genuinely silent chip still times out") {
        chip.field_on_script = { IRQType::none };
        const auto start = sys.now_ms();
        CHECK(reader.try_field_up(0) == std::unexpected(nfcv::Error::timeout));
        CHECK(sys.now_ms() > start);
    }
}

TEST_CASE("interrupts that arrive early are not lost", "[st25r39xxb][interrupts]") {
    FakeChip chip;
    FakeSys sys { chip };
    st25r39xxb::ST25R39XXB reader { chip, sys };

    SECTION("a whole exchange latched at once costs one transmission") {
        // A tag that answers promptly leaves tx_end, rx_start and rx_end set together, and one
        // status read returns all three (DS13541 §4.3.1). The await for tx_end must not throw the
        // other two away - the awaits that want them come after it.
        chip.transmit_script = { IRQType::tx_end | IRQType::rx_start | IRQType::rx_end };
        chip.response_script = { good_block_response() };

        // Read the answer on the first try, rather than timing out on rx_start or rx_end and retrying
        CHECK(read_a_block(reader).has_value());
        CHECK(chip.command_count(Command::transmit_without_crc) == 1);
    }

    SECTION("an exchange arriving one bit at a time also costs one transmission") {
        chip.transmit_script = { IRQType::tx_end, IRQType::rx_start, IRQType::rx_end };
        chip.response_script = { good_block_response() };

        CHECK(read_a_block(reader).has_value());
        CHECK(chip.command_count(Command::transmit_without_crc) == 1);
    }

    SECTION("a tag that never answers is still a no_response, on every attempt") {
        chip.transmit_script = { IRQType::tx_end };

        CHECK(read_a_block(reader) == std::unexpected(nfcv::Error::no_response));
        CHECK(chip.command_count(Command::transmit_without_crc) == st25r39xxb::ST25R39XXB::nfcv_command_max_attempts);
    }

    SECTION("a rx_end left over from the previous attempt does not satisfy the next one") {
        // Each attempt restarts the exchange, so anything still pending belongs to the one before.
        chip.transmit_script = { IRQType::tx_end };
        chip.latch(IRQType::rx_start | IRQType::rx_end);

        CHECK(read_a_block(reader) == std::unexpected(nfcv::Error::no_response));
    }
}

TEST_CASE("error interrupts are reported once", "[st25r39xxb][interrupts]") {
    FakeChip chip;
    FakeSys sys { chip };
    st25r39xxb::ST25R39XXB reader { chip, sys };

    // A framing error is not silence, so it is retried, and the error bit must be consumed by the
    // await that reported it - otherwise every later attempt fails on the same stale bit.
    chip.transmit_script = { IRQType::tx_end | IRQType::crc_error };

    CHECK(read_a_block(reader) == std::unexpected(nfcv::Error::device_crc_error));
    CHECK(chip.command_count(Command::transmit_without_crc) == st25r39xxb::ST25R39XXB::nfcv_command_max_attempts);
}
