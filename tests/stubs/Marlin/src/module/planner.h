#pragma once

// Cut-down stand-in for the planner. There are no steppers in the tests, so
// there is no position to take back; the position the planner reports is
// whatever the test sets.

#include <core/types.h>
#include <module/motion.h>

inline struct Planner {
    static inline float mm_per_step[4] = { 1, 1, 1, 1 };
    static inline MachinePosXYZE stub_position;

    static bool processing() { return false; }
    static bool draining() { return false; }
    static MachinePosXYZE get_machine_position_mm() { return stub_position; }
    static void reset_position() {}
} planner;
