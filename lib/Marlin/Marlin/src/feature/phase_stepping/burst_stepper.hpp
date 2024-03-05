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
 * @returns False if the burst can't be scheduled yet, True on success
 **/
bool fire();

/**
 * Return true if burst activity is undergoing
 * NOTE: this is only sufficient when called _after_ checking phase_stepping to ensure that a new
 *       burst can't be scheduled!
 **/
bool busy();

}; // namespace burst_stepping
