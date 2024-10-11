/**
 * @file
 */
#include "../../lib/Marlin/Marlin/src/gcode/queue.h"
#include "PrusaGcodeSuite.hpp"
#include <feature/prusa/restore_z.h>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M999: Reset MCU <a href="https://reprap.org/wiki/G-code#M999:_Restart_after_being_stopped_by_error">M999: Restart after being stopped by error</a>
 *
 *#### Usage
 *
 *    M999 [ R | Z ]
 *
 *#### Parameters
 *
 * - `R` - reset MCU
 * - `Z` - Wait for finishing planned moves, save Z coordinate and restore it after reset.
 *       - Must be combined with R parameter, doesn't work otherwise.
 *       - Z is restored only if USB flash drive is present.
 *       This strange requirement is due to coupling with power panic.
 */
void PrusaGcodeSuite::M999() {
    if (parser.seen('Z')) {
        restore_z::store();
    }
    queue.ok_to_send();
    osDelay(1000);
    NVIC_SystemReset();
}

/** @}*/
