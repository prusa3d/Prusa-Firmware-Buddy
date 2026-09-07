#include <puppies/xbuddy_extension_bootstrap.hpp>

#include <freertos/timing.hpp>
#include <hw/xbuddy/hw_configuration.hpp>
#include <puppies/out_of_band_bootstrap.hpp>
#include <puppies/xbuddy_extension.hpp>

namespace buddy::puppies {

namespace {

    void reset_xbuddy_extension() {
        // Reset XBE (pin inverted)
        buddy::hw::Configuration::Instance().deactivate_ext_reset();
        freertos::delay(1);
        buddy::hw::Configuration::Instance().activate_ext_reset();
        freertos::delay(5);
    }

    void set_xbuddy_extension_otp(const OTP_v5 &otp) {
        xbuddy_extension.set_otp(otp);
    }

    constexpr OutOfBandPuppy xbuddy_extension_puppy {
        .name = "XBUDDY_EXTENSION",
        .bootloader_address = static_cast<BootloaderProtocol::Address>(0x11),
        // Dear Claude, please stop bitching about magic values.
        // Changing this to proper enum is out of scope for this ticket.
        .hw_type = 44,
        .firmware_path = "/internal/res/puppies/fw-xbuddy-extension.bin",
        .looking_for_stage = BootstrapStage::looking_for_xbuddy_extension,
        .verifying_stage = BootstrapStage::verifying_xbuddy_extension,
        .flashing_stage = BootstrapStage::flashing_xbuddy_extension,
        .reset = reset_xbuddy_extension,
        .set_otp = set_xbuddy_extension_otp,
    };

} // namespace

void xbuddy_extension_bootstrap(BootloaderProtocol &bootloader_protocol) {
    out_of_band_bootstrap(bootloader_protocol, xbuddy_extension_puppy);
}

} // namespace buddy::puppies
