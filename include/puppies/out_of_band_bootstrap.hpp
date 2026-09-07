/// @file
#pragma once

#include <buddy/bootstrap_state.hpp>
#include <otp/types.hpp>
#include <puppies/BootloaderProtocol.hpp>

namespace buddy::puppies {

/// Everything that differs between puppies brought up out-of-band.
///
/// Out-of-band means ahead of the standard PuppyBootstrap::run() DOCKS loop.
/// Such a puppy gates bus access to a child puppy on the same RS-485 bus
/// (xBuddy Extension -> INDX head, XL-CAN bridge -> Modular Bed), so it has
/// to be running its firmware before the standard discover/flash phases can
/// reach the child. It is consequently not a DOCKS member and never flows
/// through the discover/flash loop.
struct OutOfBandPuppy {
    /// Reported in the fatal_error() messages raised during the bring-up.
    const char *name;

    /// Address the puppy's bootloader answers at.
    BootloaderProtocol::Address bootloader_address;

    /// hw_type the bootloader has to report.
    uint8_t hw_type;

    /// Path of the firmware image in the internal filesystem.
    const char *firmware_path;

    BootstrapStage looking_for_stage;
    BootstrapStage verifying_stage;
    BootstrapStage flashing_stage;

    /// Drives the puppy's reset line so that it re-enters its bootloader, and
    /// returns once it is ready to answer. Called before every handshake
    /// attempt: the puppy is not in the master's reset domain, so after an
    /// earlier run_app it sits at its application address and bootloader
    /// queries would never be answered.
    void (*reset)();

    /// Hands the OTP block read from the bootloader over to the puppy's driver.
    void (*set_otp)(const OTP_v5 &);
};

/// One bounded handshake round at the puppy's bootloader address: pulse
/// reset(), ask for the protocol version, a few attempts. Reports whether the
/// puppy answered.
///
/// Split out of out_of_band_bootstrap() for callers that tolerate an absent
/// puppy and therefore cannot use its fatal_error-on-everything policy -- an
/// XLS bridge probe has to accept silence as "this is a plain XL".
[[nodiscard]] bool out_of_band_discover(BootloaderProtocol &, const OutOfBandPuppy &, uint16_t &protocol_version);

/// Brings the puppy from its bootloader into its firmware: handshake,
/// hw_type and OTP read, verify/flash, run_app.
///
/// Every failure is fatal, including an absent puppy -- a call site that has
/// to survive one must gate on out_of_band_discover() first.
void out_of_band_bootstrap(BootloaderProtocol &, const OutOfBandPuppy &);

} // namespace buddy::puppies
