/**
 * @file gui_config_printer.hpp
 * @brief iX printer specific gui config
 */

#pragma once

#include <cstdint>

// axis length [mm]
//! @todo Copy pasted from XL, update with iX values.
#define X_LEN 360
#define Y_LEN 345
#define Z_LEN 370

// tolerance (common for all axes)
#define LEN_TOL_ABS 15 // length absolute tolerance (+-5mm)
#define LEN_TOL_REV 13 // length tolerance in reversed direction (3mm)
