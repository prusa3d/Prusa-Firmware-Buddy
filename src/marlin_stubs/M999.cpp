/**
 * @file
 */
#include "../../lib/Marlin/Marlin/src/gcode/queue.h"
#include "PrusaGcodeSuite.hpp"
#include <feature/prusa/restore_z.h>

/**
 * @brief reset MCU.
 *
 * Prusa STM32 platform specific
 *
 *
 * ## Parameters
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
