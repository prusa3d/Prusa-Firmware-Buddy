/**
 * @file calibration_z.hpp
 * @author Radek Vana
 * @brief gives access to Z calibration
 * @date 2021-12-09
 */
#pragma once

#include <option/has_uneven_bed_detection.h>

namespace selftest {
/// Z align calibration, after user was notified.
void calib_Z(bool move_down_after);

#if HAS_UNEVEN_BED_DETECTION()
/// Z align calibration that can happen without users intervention
/// Checks for obstructions in Z axis
/// @return true - Z aligned, false - unable to align
bool auto_calib_Z(bool move_down_after);
#endif // HAS_UNEVEN_BED_DETECTION()
} // namespace selftest
