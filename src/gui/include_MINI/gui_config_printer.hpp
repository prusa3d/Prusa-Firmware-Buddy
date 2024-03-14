/**
 * @file gui_config_printer.hpp
 * @brief MINI printer specific gui config
 */

#pragma once

#include <cstdint>

// axis length [mm]
inline constexpr uint8_t X_LEN = 184;
inline constexpr uint8_t Y_LEN = 183;
inline constexpr uint8_t Z_LEN = 186;

// tolerance (common for all axes)
inline constexpr uint8_t LEN_TOL_ABS = 15; // length absolute tolerance (+-5mm)
inline constexpr uint8_t LEN_TOL_REV = 13; // length tolerance in reversed direction (3mm)

// static const float Z_OFFSET_STEP = 0.0025F; //calculated
inline constexpr float Z_OFFSET_MIN = -2.0F;
inline constexpr float Z_OFFSET_MAX = 0.0F;
