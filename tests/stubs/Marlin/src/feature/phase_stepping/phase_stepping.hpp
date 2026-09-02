#pragma once

// Cut-down stand-in for phase stepping, which the tests never enable.

namespace phase_stepping {

inline bool any_axis_enabled() { return false; }

} // namespace phase_stepping
