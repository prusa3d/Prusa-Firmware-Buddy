/**
 * @file
 */
#include "restore_z.h"
#include "../../module/planner.h"
#include "../../module/motion.h"

/**
 * @brief Restoring of Z coordinate is requested
 */
static bool is_requested() {
    return isfinite(config_store().restore_z_after_boot.get().current_position_z);
}

/**
 * @brief Restore Z coordinate if stored and clear storage.
 *
 * Caller is responsible for ensuring there is no ongoing action which may be affected
 * by changing planner and stepper position. E.g. this should not be called in the middle
 * of print.
 */
void restore_z::restore() {
    if (!is_requested()) {
        return;
    }
    current_position[Z_AXIS] = config_store().restore_z_after_boot.get().current_position_z;
    planner.set_position_mm(current_position);
    if (TEST(config_store().restore_z_after_boot.get().axis_known_position, Z_AXIS)) {
        SBI(axis_known_position, Z_AXIS);
        SBI(axis_homed, Z_AXIS);
    }

    restore_z::clear();
}

/**
 * @brief Wait for finishing planned moves and store Z coordinate
 */
void restore_z::store() {
    planner.synchronize();
    config_store().restore_z_after_boot.set({ current_position[Z_AXIS], axis_known_position });
}
