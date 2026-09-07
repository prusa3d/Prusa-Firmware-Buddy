#pragma once

#include <concepts>
#include <utility>

#include <prusa3d/common/SharedFault_1_0.h>
#include <bsod/bsod.h>

namespace puppy::fault {
///@brief enumeration of shared faults that puppies can have
enum SharedFault {
    unknown = prusa3d_common_SharedFault_1_0_FAULT_UNKNOWN,
    heartbeat_missing = prusa3d_common_SharedFault_1_0_FAULT_HEARTBEAT_MISSING,
    data_timeout = prusa3d_common_SharedFault_1_0_FAULT_DATA_TIMEOUT,
    pcb_overheat = prusa3d_common_SharedFault_1_0_FAULT_PCB_OVERHEAT,
    mcu_overheat = prusa3d_common_SharedFault_1_0_FAULT_MCU_OVERHEAT,
    can = prusa3d_common_SharedFault_1_0_FAULT_CAN,

    /// This is a barrier for puppy specific faults
    _shared_first = prusa3d_common_SharedFault_1_0_FAULT_SHARED_FIRST,
};

template <typename E>
concept SpecificFaultType = std::is_enum_v<E> && requires(E e) {
    E::_specific_last;

    E::can;
    E::pcb_overheat;
    E::mcu_overheat;
    E::data_timeout;
    E::heartbeat_missing;
    E::unknown;
};

/// @brief Convert Shared fault to Specific.
template <SpecificFaultType E>
static constexpr E from_shared(SharedFault fault) {
    /// @note This switch will throw a warning if not all shared faults are handled.
    switch (fault) {
    case SharedFault::unknown:
        static_assert(std::to_underlying(SharedFault::unknown) == std::to_underlying(E::unknown));
        break;
    case SharedFault::pcb_overheat:
        static_assert(std::to_underlying(SharedFault::pcb_overheat) == std::to_underlying(E::pcb_overheat));
        return E::pcb_overheat;
    case SharedFault::mcu_overheat:
        static_assert(std::to_underlying(SharedFault::mcu_overheat) == std::to_underlying(E::mcu_overheat));
        return E::mcu_overheat;
    case SharedFault::heartbeat_missing:
        static_assert(std::to_underlying(SharedFault::heartbeat_missing) == std::to_underlying(E::heartbeat_missing));
        return E::heartbeat_missing;
    case SharedFault::data_timeout:
        static_assert(std::to_underlying(SharedFault::data_timeout) == std::to_underlying(E::data_timeout));
        return E::data_timeout;
    case SharedFault::can:
        static_assert(std::to_underlying(SharedFault::can) == std::to_underlying(E::can));
        return E::can;
    case SharedFault::_shared_first: // Remove if this code is covered by some real value
        break;
    }
    debug_assert(false);
    return E::unknown;
    static_assert(std::to_underlying(SharedFault::_shared_first) > std::to_underlying(E::_specific_last));
}

/// @brief Trigger shared fault
bool trigger_fault(SharedFault);

} // namespace puppy::fault
