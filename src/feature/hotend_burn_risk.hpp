/// @file
#pragma once

namespace buddy {

/// Show or clear the hotend burn-risk warning from the live door + nozzle state.
/// Called every marlin_server::cycle(). Deliberately independent of EmergencyStop:
/// the burn hazard exists whether or not the emergency-stop feature is enabled.
void check_hotend_burn_risk();

} // namespace buddy
