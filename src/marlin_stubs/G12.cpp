#include "PrusaGcodeSuite.hpp"

#include <nozzle_cleaner.hpp>
#include <logging/log.hpp>
#include "common/gcode/inject_queue_actions.hpp"
#include "marlin_server.hpp"

LOG_COMPONENT_REF(PRUSA_GCODE);

/** \addtogroup G-Codes
 * @{
 */

/**
 *### G12: Clean nozzle on Nozzle Cleaner <a href="https://reprap.org/wiki/G-code#G12:_Clean_Tool">G12: Clean Tool</a>
 *
 * Only iX
 *
 * #### Usage
 *
 *     G12
 *
 */

void PrusaGcodeSuite::G12() {
    marlin_server::inject({ GCodeFilename(nozzle_cleaner::g12_filename, nozzle_cleaner::g12_sequence) });
}

/** @}*/
