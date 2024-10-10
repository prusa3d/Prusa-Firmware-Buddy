#include "PrusaGcodeSuite.hpp"

#include <logging/log.hpp>
#include "common/gcode/inject_queue_actions.hpp"
#include "marlin_server.hpp"

LOG_COMPONENT_REF(PRUSA_GCODE);

/** \addtogroup G-Codes
 * @{
 */

/**
 * G12: Clean nozzle on Nozzle Cleaner
 *
 */

void PrusaGcodeSuite::G12() {
    // TODO: Check file exists and if not, execute a hard-coded default cleaning sequence.
    marlin_server::inject({ GCodeFilename("nozzle_cleaning_sequence") });
}

/** @}*/
