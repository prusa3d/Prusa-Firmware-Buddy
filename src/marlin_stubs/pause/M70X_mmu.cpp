
#include "config_features.h"
#include "../../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#include "M70X.hpp"

#include <filament_sensors_handler.hpp>

/**
 * @brief load filament to MMU
 *
 * @param data slot index
 */
void filament_gcodes::mmu_load(uint8_t data) {
    MMU2::mmu2.load_filament(data);
}

void filament_gcodes::mmu_load_test(uint8_t data) {
    MMU2::mmu2.loading_test(data);
}

/**
 * @brief eject filament from mmu
 *
 * @param data slot index
 */
void filament_gcodes::mmu_eject(uint8_t data) {
    MMU2::mmu2.eject_filament(data, false); //@@TODO tune "recover" parameter
}

/**
 * @brief cut filament with mmu
 *
 * @param data slot index
 */
void filament_gcodes::mmu_cut(uint8_t data) {
    MMU2::mmu2.cut_filament(data);
}

/**
 * @brief do mmu reset
 *
 */
void filament_gcodes::mmu_reset(uint8_t level) {
    MMU2::mmu2.Reset(MMU2::MMU2::ResetForm(level));
}

/**
 * @brief turn mmu on
 *
 */
void filament_gcodes::mmu_on() {
    config_store().mmu2_enabled.set(true);
    MMU2::mmu2.Start();
}

/**
 * @brief turn mmu off
 *
 */
void filament_gcodes::mmu_off() {
    config_store().mmu2_enabled.set(false);

    // MMU uses config_store.filament_type to store what's preloaded in the MMU
    // When we turn the MMU off, it should now actually store what's in the nozzle.
    // If there is nothing in the nozzle, clear the setting.
    // BFW-5199
    if (!FSensors_instance().has_filament(true)) {
        config_store().filament_type_0.set(filament::Type::NONE);
    }

    MMU2::mmu2.Stop();
}
