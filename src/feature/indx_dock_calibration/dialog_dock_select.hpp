/// @file
#pragma once

#include <optional>
#include <cstdint>

#include "dock_calibration_data.hpp"

/// Shows a dialog that prompts the user to pick a per-dock action (keep/calibrate/invalidate)
/// @param dock_count number of docks to show (1-8)
/// @param preselect_all if true, default every dock to Calibrate; otherwise default only uncalibrated docks to Calibrate
/// @returns per-dock actions, or std::nullopt if aborted
std::optional<DockSelection> select_docks_dialog(uint8_t dock_count, bool preselect_all);
