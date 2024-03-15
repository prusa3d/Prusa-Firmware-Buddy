/**
 * @file mmu_state.h
 * @brief status of mmu
 */
#pragma once
#include <stdint.h>
namespace MMU2 {
/// States of a printer with the MMU:
/// - Active
/// - Connecting
/// - Stopped
///
/// When the printer's FW starts, the MMU mode is either Stopped or NotResponding (based on user's preference).
/// When the MMU successfully establishes communication, the state changes to Active.
enum class xState : uint_fast8_t {
    /// The user doesn't want the printer to work with the MMU. The MMU itself is not powered and does not work at all.
    /// !!! Must be 0 !!! marlin_vars.mmu2_state is set to 0 if not active
    Stopped,

    Active, ///< MMU has been detected, connected, communicates and is ready to be worked with.
    Connecting, ///< MMU is connected but it doesn't communicate (yet). The user wants the MMU, but it is not ready to be worked with.
    Bootloader, ///< Trying to communicate with the MMU bootloader
};

} // namespace MMU2
