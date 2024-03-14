/*
 * CoreXY precise homing refinement
 * TODO: @wavexx: Add some documentation
 */

#pragma once
#include "../../core/types.h"

// CoreXY support functions
void corexy_ab_to_xy(const xy_long_t &steps, xy_pos_t &mm);
void corexy_ab_to_xyze(const xy_long_t &steps, xyze_pos_t &mm);

// CoreXY precise refinement
bool refine_corexy_origin();
