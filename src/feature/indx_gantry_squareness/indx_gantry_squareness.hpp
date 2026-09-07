/// @file
#pragma once

namespace indx_gantry_squareness {

/// Maximum gantry skew over the dock span still considered square [mm]
inline constexpr float dy_limit_mm = 0.5f;

/// Gantry squareness wizard (M1988): parks the tool, asks the user to empty
/// the outermost docks, then measures the skew by stalling the empty head
/// against them. Persists the pass/fail result to selftest_result_gantry_squareness.
void run_wizard();

/// The same measurement without GUI (M1988 S), for unattended testing.
/// Does not touch the stored result; the result goes to the log.
void run_silent();

} // namespace indx_gantry_squareness
