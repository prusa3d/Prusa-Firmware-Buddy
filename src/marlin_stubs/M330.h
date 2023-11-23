/**
 * @file
 */
#pragma once

namespace PrusaGcodeSuite {

/// @name Metric related GCodes.
///
/// TODO: This is WIP. Those Gcodes need redesign.
/// @{
void M330(); ///< select handler
void M331(); ///< enable metric
void M332(); ///< disable metric
void M333(); ///< print metrics and their settings for selected handler
void M334(); ///< handler-specific configuration
/// @}
} // namespace PrusaGcodeSuite
