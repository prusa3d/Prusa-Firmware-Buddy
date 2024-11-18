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
#define BB(NAME) static_cast<uint16_t>(FaultStatusMask::NAME)
    return err & (BB(TMC_RESET) | BB(TMC_UNDERVOLTAGE) | BB(TMC_SHORT) | BB(TMC_OVERHEAT) | BB(TMC_OTHER));
#undef BB
}

} // namespace dwarf_shared::errors
