/**
 * @file
 */
#pragma once

namespace PrusaGcodeSuite {

/// @name Metric related GCodes.
/// @{
void M331(); //< enable metric
void M332(); //< disable metric
void M333(); //< print metrics and their settings
void M334(); //< configure metrics & syslog
/// @}

/// Prompts the user about metrics configuration change.
/// \returns if the user agrees with the changes
bool metrics_config_change_prompt();

} // namespace PrusaGcodeSuite
