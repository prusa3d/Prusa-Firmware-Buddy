#pragma once

enum class MMU2BootloaderResult {
    /// Bootloader was not detected
    not_detected,

    /// Bootloader detected, firmware up to date
    fw_up_to_date,

    /// A new firmware was successfully flashed
    fw_updated,

    /// Error during firmware flashing
    flashing_error,

    /// Bootloader detected, but had a communication error
    comm_error,
};
