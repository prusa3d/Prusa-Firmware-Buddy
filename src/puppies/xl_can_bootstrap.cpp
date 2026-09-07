#include <puppies/xl_can_bootstrap.hpp>

#include <buddy/bootstrap_state.hpp>
#include <freertos/timing.hpp>
#include <hwio_pindef.h>
#include <logging/log.hpp>
#include <puppies/out_of_band_bootstrap.hpp>
#include <puppies/xl_can.hpp>

LOG_COMPONENT_REF(Puppies);

namespace buddy::puppies {

using buddy::BootstrapStage;
using buddy::hw::Pin;

namespace {

    void reset_xl_can() {
        // Reset the bridge into its bootloader before each handshake attempt: the
        // bridge MCU is NOT in xlBuddy's reset domain, so after a previous
        // successful run_app the bridge sits at the app address and bootloader
        // queries would silently NO_RESPONSE forever.
        //
        // Polarity (reworked v02 NRST circuit): the bridge resets on a HIGH->LOW
        // edge of modular_bed_reset (PCA9557 p7); the line must idle HIGH to arm
        // the edge network. So: assert LOW, release back HIGH, then give the
        // bridge time to boot before the bootloader query. Timings are
        // bench-verified (BFW-8797).
        buddy::hw::modular_bed_reset.write(Pin::State::low);
        freertos::delay(2);
        buddy::hw::modular_bed_reset.write(Pin::State::high);
        freertos::delay(50);
    }

    void set_xl_can_otp(const OTP_v5 &otp) {
        xl_can.set_otp(otp);
    }

    constexpr OutOfBandPuppy bridge {
        .name = "XL_CAN",
        // The XL-CAN bootloader (BOARD_TYPE_prusa_xl_can) answers at a dedicated
        // bootloader address and reports its own hw_type, so a real XL-CAN is
        // distinguished from an xBE (0x11 / hw_type 44) on the shared bus.
        .bootloader_address = static_cast<BootloaderProtocol::Address>(0x13),
        .hw_type = 46,
        // Path matches add_resource("${XL_CAN_BINARY_PATH}",
        // "/puppies/fw-xl_can.bin") in src/resources/CMakeLists.txt.
        .firmware_path = "/internal/res/puppies/fw-xl_can.bin",
        .looking_for_stage = BootstrapStage::looking_for_xl_can,
        .verifying_stage = BootstrapStage::verifying_xl_can,
        .flashing_stage = BootstrapStage::flashing_xl_can,
        .reset = reset_xl_can,
        .set_otp = set_xl_can_otp,
    };

    /// Raise modular_bed_reset HIGH and hold so the bridge NRST edge network
    /// arms before the first reset_bridge() pulse. The pindef initializes the
    /// pin LOW (plain-XL idle level); without this pre-arm the HIGH->LOW pulse
    /// fires on the wrong edge and the bootloader probe is non-deterministic.
    void pre_arm_xl_can_reset() {
        buddy::hw::modular_bed_reset.write(Pin::State::high);
        freertos::delay(100);
    }

} // namespace

bool xl_can_probe(BootloaderProtocol &bootloader_protocol) {
    bootstrap_state_set(0, BootstrapStage::looking_for_xl_can);

    pre_arm_xl_can_reset();

    bootloader_protocol.set_address(bridge.bootloader_address);

    uint16_t protocol_version;

    if (out_of_band_discover(bootloader_protocol, bridge, protocol_version)) {
        log_info(Puppies, "XL-CAN probe: present - XLS");
        // The line stays HIGH from the last reset_bridge() pulse; xl_can_bootstrap()
        // re-arms from HIGH, which is a no-op for the edge network.
        return true;

    } else {
        log_info(Puppies, "XL-CAN probe: absent (plain XL)");
        buddy::hw::modular_bed_reset.write(Pin::State::low);
        return false;
    }
}

void xl_can_bootstrap(BootloaderProtocol &bootloader_protocol) {
    pre_arm_xl_can_reset();
    out_of_band_bootstrap(bootloader_protocol, bridge);
}

} // namespace buddy::puppies
