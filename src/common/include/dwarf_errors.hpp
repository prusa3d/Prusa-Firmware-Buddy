#pragma once

#include <cstdint>

/**
 * @brief Shared between buddy and dwarf
 */
namespace dwarf_shared::errors {
enum class FaultStatusMask : uint16_t {
    NO_FAULT = 0,
    MARLIN_KILLED = (1 << 1),
    TMC_FAULT = (1 << 2),
};

} // namespace dwarf_shared::errors
