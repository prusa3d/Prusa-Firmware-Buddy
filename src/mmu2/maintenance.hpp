#pragma once

#include <client_fsm_types.h>

#include <optional>

namespace MMU2 {

enum class MaintenanceReason {
    /// Too many filament changes.
    Changes,
    /// Failed often lately.
    Failures,
};

/// Shall we recommend the extruder maintenance to the user?
std::optional<MaintenanceReason> check_maintenance();

} // namespace MMU2
