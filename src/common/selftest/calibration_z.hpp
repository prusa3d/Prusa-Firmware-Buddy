/**
 * @file calibration_z.hpp
 * @author Radek Vana
 * @brief gives access to Z calibration
 * @date 2021-12-09
 */
#pragma once

namespace selftest {
bool calib_Z(bool move_down_after, bool prevent_collision = false);
}
