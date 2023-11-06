#pragma once

#include "../../inc/MarlinConfig.h"
#include <option/has_burst_stepping.h>

// This module provides interface for generating given number of steps with as
// least CPU intervention as possible.

namespace burst_stepping {

/**
 * Initializes burst-step generator. This function has to be called before any
 * other function of burst stepping is invoked.
 **/
void init();

/**
 * Sets number of step pulses to generate for given axis. The steps are not
 * generated until fire is invoked.
 **/
void set_phase_diff(AxisEnum axis, int diff);

/**
 * Starts asynchronously generating steps previously set via set_phase_diff;
 **/
void fire();

}; // namespace burst_stepping
