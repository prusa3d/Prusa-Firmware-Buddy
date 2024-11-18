#pragma once

#include <cstdint>

/**
 * @brief Shared between buddy and dwarf
 */
namespace dwarf_shared::errors {
enum class FaultStatusMask : uint16_t {
    NO_FAULT = 0,
    MARLIN_KILLED = (1 << 1),
    TMC_RESET = (1 << 2),
    TMC_UNDERVOLTAGE = (1 << 3),
    TMC_SHORT = (1 << 4),
    TMC_OVERHEAT = (1 << 5),
    TMC_OTHER = (1 << 6),
};

inline bool is_tmc_error(uint16_t err) {
#define B(NAME) static_cast<uint16_t>(FaultStatusMask::NAME)
    return err & (B(TMC_RESET) | B(TMC_UNDERVOLTAGE) | B(TMC_SHORT) | B(TMC_OVERHEAT) | B(TMC_OTHER));
#undef B
}

} // namespace dwarf_shared::errors
