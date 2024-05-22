#include <feature/input_shaper/input_shaper_config.hpp>
#include <module/planner.h>

#include "PrusaGcodeSuite.hpp"

/**
 * @brief Reload Input Shaper settings from config store
 * Fix for BFW-5251
 * Beware, resets mass set by M74 - to fix that, larger refactoring is needed: BFW-5271
 *
 * !!! For internal use only, can be changed or removed at any time
 */
void PrusaGcodeSuite::M9200() {
    // For now, we must ensure that all queues are empty before changing input shapers parameters.
    // But later, it could be possible to wait just for block and move quests.
    planner.synchronize();

    input_shaper::init();
}
