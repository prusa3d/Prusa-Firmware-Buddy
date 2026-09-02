#pragma once

// Cut-down stand-in for Marlin.h, providing just what signal2step needs. There
// is no Marlin task in the tests, so there is nothing to pump.

inline void idle(bool) {}

inline void enable_all_steppers() {}
