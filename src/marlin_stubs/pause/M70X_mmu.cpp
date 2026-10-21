
#include "config_features.h"
#include "../../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#include "M70X.hpp"

#include <feature/filament_sensor/filament_sensors_handler.hpp>

#include <option/has_mmu2.h>
#include <option/has_anfc_for_multiple_tools.h>

/**
 * @brief load filament to MMU
 *
 * @param data slot index
 */
void filament_gcodes::mmu_load(uint8_t data) {
    MMU2::mmu2.load_filament(data);
}

void filament_gcodes::mmu_load_test(VirtualToolIndex slot) {
    MMU2::mmu2.loading_test(slot.to_raw());
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

#if HAS_ANFC() && !HAS_ANFC_FOR_MULTIPLE_TOOLS()
    // Remove link to a NFC Tag
    for (const VirtualToolIndex tool : VirtualToolIndex::all()) {
        FilamentType { AdHocFilamentType { tool } }.modify_parameters([](FilamentTypeParameters &p) {
            p.openprinttag_uid_hash = buddy::openprinttag::ToolTag::no_tag_hash;
        });
    }
#endif

    MMU2::mmu2.Start();

    // To ensure SideFS (MMU's FS) is properly set up in FS handler
    // logical_sensor_states_ has to update with logical filament sensors already reconfigured and enabled
    FSensors_instance().request_enable_state_update();
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
    if (!FSensors_instance().has_filament_surely(LogicalFilamentSensor::extruder)) {
        config_store().set_filament_type(VirtualToolIndex::from_raw(0), {});
    }

    MMU2::mmu2.Stop();

    // Update logical FS states
    FSensors_instance().request_enable_state_update();
}
