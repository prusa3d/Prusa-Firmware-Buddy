#include <feature/input_shaper/input_shaper_config.hpp>
#include <module/planner.h>

#include "PrusaGcodeSuite.hpp"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M9200: Reload Input Shaper settings from config store
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    M9200
 *
 */
void PrusaGcodeSuite::M9200() {
    // For now, we must ensure that all queues are empty before changing input shapers parameters.
    // But later, it could be possible to wait just for block and move quests.
    planner.synchronize();

    input_shaper::init();
}
/** @}*/
