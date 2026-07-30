#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "fake_chip.hpp"

#include <algorithm>
#include <limits>

using fake::FakeChip;
using fake::FakeSys;
using st25r39xxb::Command;
using st25r39xxb::IRQType;
using st25r39xxb::RegisterA;

namespace {

const auto modulation = st25r39xxb::ModulationConfiguration { st25r39xxb::config::OOKModulation { { std::nullopt } } };

} // namespace

TEST_CASE("try_field_up reports a collision distinctly", "[st25r39xxb][field_up]") {
    FakeChip chip;
    FakeSys sys { chip };
    st25r39xxb::ST25R39XXB reader { chip, sys };

    SECTION("collision_detected becomes field_collision") {
        chip.field_on_script = { IRQType::collision_detected };
        CHECK(reader.try_field_up(0) == std::unexpected(nfcv::Error::field_collision));
    }

    SECTION("no IRQ at all is still a plain timeout") {
        chip.field_on_script = { IRQType::none };
        CHECK(reader.try_field_up(0) == std::unexpected(nfcv::Error::timeout));
    }

    SECTION("a collision leaves nothing radiating") {
        chip.field_on_script = { IRQType::collision_detected };
        REQUIRE(!reader.try_field_up(0).has_value());
        CHECK(!chip.field_bits_set());
    }

    SECTION("success sets the field bits and re-arms the receiver") {
        chip.field_on_script = { IRQType::pwp_field_active };
        REQUIRE(reader.try_field_up(0).has_value());
        CHECK(chip.field_bits_set());
        CHECK(chip.command_count(Command::stop_all_1) == 1);
        CHECK(chip.command_count(Command::reset_rx_gain) == 1);
    }

    SECTION("a stale collision_detected does not cause a false collision") {
        chip.latch(IRQType::collision_detected);
        chip.field_on_script = { IRQType::pwp_field_active };
        CHECK(reader.try_field_up(0).has_value());
    }

    SECTION("a collision latched alongside the field-on still loses the round") {
        // One interrupt read can return several bits (DS13541 §4.3.1). The collision has to be the
        // one that decides, or we radiate into a neighbour's field and call it a won arbitration.
        chip.field_on_script = { IRQType::pwp_field_active | IRQType::collision_detected };
        CHECK(reader.try_field_up(0) == std::unexpected(nfcv::Error::field_collision));
        CHECK(!chip.field_bits_set());
    }

    SECTION("the same antenna already up is a no-op") {
        chip.field_on_script = { IRQType::pwp_field_active };
        REQUIRE(reader.try_field_up(1).has_value());
        REQUIRE(chip.field_on_count == 1);

        REQUIRE(reader.try_field_up(1).has_value());
        CHECK(chip.field_on_count == 1);
    }
}

TEST_CASE("init_nfcv_poller does not arbitrate and leaves the field down", "[st25r39xxb][field_up]") {
    FakeChip chip;
    FakeSys sys { chip };
    st25r39xxb::ST25R39XXB reader { chip, sys };

    SECTION("init never brings the field up, whatever the chip would answer") {
        // Arbitration belongs to the first discovery pass (nfcv::FieldManager), not to the boot:
        // the caller panics when init fails, so a round lost to a neighbour's field would brick
        // the board. Since no field-on is issued at all, the script cannot influence the outcome.
        chip.field_on_script = { GENERATE(IRQType::pwp_field_active, IRQType::collision_detected, IRQType::none) };

        CHECK(reader.init_nfcv_poller(modulation).has_value());
        CHECK(chip.field_on_count == 0);
        CHECK(!chip.field_bits_set());
    }

    SECTION("the external field detector is armed with our own thresholds") {
        REQUIRE(reader.init_nfcv_poller(modulation).has_value());

        const auto activation = chip.reg_a(RegisterA::external_field_detector_activation_threshold);
        const auto deactivation = chip.reg_a(RegisterA::external_field_detector_deactivation_threshold);

        // The reset 205 mV is too deaf to notice a neighbouring board, so it must not survive init
        CHECK(activation != FakeChip::efd_activation_reset);
        CHECK(deactivation != FakeChip::efd_deactivation_reset);

        static constexpr std::byte collision_nibble { 0x0f };
        CHECK((activation & collision_nibble) == std::byte { st25r39xxb::ST25R39XXB::efd_collision_activation_code });
        CHECK((deactivation & collision_nibble) == std::byte { st25r39xxb::ST25R39XXB::efd_collision_deactivation_code });

        // Peer detection is unused in reader mode - leave its nibbles as the chip reset them
        static constexpr std::byte peer_nibble { 0xf0 };
        CHECK((activation & peer_nibble) == (FakeChip::efd_activation_reset & peer_nibble));
        CHECK((deactivation & peer_nibble) == (FakeChip::efd_deactivation_reset & peer_nibble));

        // en_fd_c<1:0> = 01b, the detector armed against the collision-avoidance threshold
        CHECK((chip.reg_a(RegisterA::operation_control) & std::byte { 0x03 }) == std::byte { 0x01 });
    }

    SECTION("the first bring-up after init genuinely selects its antenna") {
        chip.field_on_script = { IRQType::pwp_field_active };
        REQUIRE(reader.init_nfcv_poller(modulation).has_value());

        const auto commands_after_init = chip.commands.size();
        REQUIRE(reader.try_field_up(1).has_value());

        // init left active_antenna unset and the field genuinely down, so this is a real bring-up
        CHECK(chip.commands.size() > commands_after_init);
        CHECK(chip.field_bits_set());
    }
}
