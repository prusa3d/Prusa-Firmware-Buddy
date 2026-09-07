#pragma once

#include <option/has_heaters_selftest_gcode.h>

#if HAS_HEATERS_SELFTEST_GCODE()

namespace heaters_selftest {

/// Runs the gcode-based heater selftest (driven by M1987): tests the nozzle and bed heaters,
/// stores the results into config_store().selftest_result. Mirrors the fan selftest (M1978).
void run();

} // namespace heaters_selftest

#endif // HAS_HEATERS_SELFTEST_GCODE()
