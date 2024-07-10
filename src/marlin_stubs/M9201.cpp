#include <module/planner.h>

#include "PrusaGcodeSuite.hpp"

/**
 * Reset motion parameters to defaults (feedrate, accelerations).
 *
 * !!! For internal use only, can be changed or removed at any time
 */
void PrusaGcodeSuite::M9201() {
    Motion_Parameters::reset();
}
