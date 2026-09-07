/// @file
/// Marlin2 interface implementation for the MMU2
#include "mmu2_marlin.h"

#include "mapi/motion.hpp"
#include "mapi/parking.hpp"

#include "../../inc/MarlinConfig.h"
#include "../../gcode/gcode.h"
#include "../../module/temperature.h"
#include "../../module/planner.h"
#include "../../module/stepper/indirection.h"
#include "../../Marlin.h"
#include "marlin_server.hpp"

#include <option/has_auto_retract.h>
#include <bsod/bsod.h>
#if HAS_AUTO_RETRACT()
    #include <feature/auto_retract/auto_retract.hpp>
#endif

namespace MMU2 {

#if PRINTER_IS_PRUSA_MK3_5()
// MK3.5 underextrudes by default to match mk3's behaviour,
// that doesn't play nicely with the absolute move distances of the MMU
// -> need to compensate for the slight discrepancy.
static constexpr float eStepsMultiplier = 1 / 0.95F;
#else
static constexpr float eStepsMultiplier = 1.0F;
#endif

void extruder_move(float distance, float feed_rate) {
    mapi::extruder_move(distance * eStepsMultiplier, feed_rate);
}

void extruder_schedule_turning(float feed_rate) {
    mapi::extruder_schedule_turning(feed_rate);
}

float stepper_get_machine_position_E_mm() {
    // it really depends on the coherence of sampling from the stepper
    // we don't need any extra precision, 1mm should be enough
    return planner.get_axis_position_mm(E_AXIS);
}

void planner_abort_queued_moves() {
    PreciseStepping::quick_stop();
    while (!planner.draining() && PreciseStepping::stopping()) {
        PreciseStepping::loop();
    }
}

bool planner_draining() {
    return planner.draining();
}

float move_raise_z(float delta) {
    // @@TODO
    return 0.0F;
}

void planner_synchronize() {
    planner.synchronize();
}

UnloadDistanceDetector::UnloadDistanceDetector() {
    fs = WhereIsFilament();
    unlFSOff = stepper_get_machine_position_E_mm();
}

void UnloadDistanceDetector::operator()() {
    if (auto currentFS = WhereIsFilament(); fs != currentFS && currentFS == FilamentState::NOT_PRESENT) {
        // fsensor just turned off, remember the E-motor stepper position
        unlFSOff -= stepper_get_machine_position_E_mm();
        fs = currentFS; // avoid further records
    }
}

void planner_synchronize_hook(UnloadDistanceDetector &udd) {
    Subscriber idle_sub {
        marlin_server::idle_publisher,
        [&] { udd(); },
    };
    planner.synchronize();
};

bool planner_any_moves() {
    return planner.processing();
}

uint8_t planner_moves_planned_count() {
    return planner.movesplanned();
}

pos3d planner_current_position() {
    return pos3d(current_position.x, current_position.y, current_position.z);
}

void motion_do_blocking_move_to_xy(float rx, float ry, float feedRate_mm_s) {
    do_blocking_move_to_xy(rx, ry, feedRate_mm_s);
}

void motion_do_blocking_move_to_z(float z, float feedRate_mm_s) {
    do_blocking_move_to_z(z, feedRate_mm_s);
}

void nozzle_park() {
    mapi::park(mapi::get_parking_position(mapi::ParkPosition::filament_change));
}

bool marlin_printingIsActive() {
    return printingIsActive();
}

void marlin_manage_heater() {
    thermalManager.manage_heater();
}

void marlin_manage_inactivity([[maybe_unused]] bool ignore_stepper_queue) {
    // ignore_stepper_queue was removed from Marlin::manage_inactivity
    // Keeping it in the MMU code to not break the compatibility with MK3
    manage_inactivity();
}

void marlin_idle(bool waiting, [[maybe_unused]] bool ignore_stepper_queue) {
    // ignore_stepper_queue was removed from Marlin::manage_inactivity
    // Keeping it in the MMU code to not break the compatibility with MK3
    idle(waiting);
}

void marlin_refresh_print_state_in_ram() {
    // @@TODO
}

void marlin_clear_print_state_in_ram() {
    // @@TODO
}

void marlin_stop_and_save_print_to_ram() {
    // @@TODO
}

void marlin_finalize_unload() {
#if HAS_AUTO_RETRACT()
    static_assert(HOTENDS == 1);

    // The filament is completely out of the nozzle - so not auto-retracted
    buddy::auto_retract().set_retracted_distance(PhysicalToolIndex::from_raw(0), std::nullopt);
#endif
}

bool marlin_is_retracted() {
#if HAS_AUTO_RETRACT()
    static_assert(HOTENDS == 1);
    return buddy::auto_retract().can_cold_unload(PhysicalToolIndex::from_raw(0));
#else
    return false;
#endif
}

int16_t thermal_degTargetHotend() {
    return match(
        PhysicalToolIndex::currently_selected(),
        [](PhysicalToolIndex physical_tool) { return thermalManager.degTargetHotend(physical_tool); },
        [](NoTool) -> int16_t {
            debug_assert(false);
            return 0;
        });
}

float thermal_degHotend() {
    return match(
        PhysicalToolIndex::currently_selected(),
        [](PhysicalToolIndex physical_tool) { return thermalManager.degHotend(physical_tool); },
        [](NoTool) {
            debug_assert(false);
            return 0.0f;
        });
}

void thermal_setTargetHotend(int16_t t) {
    match(
        PhysicalToolIndex::currently_selected(),
        [t](PhysicalToolIndex physical_tool) { thermalManager.setTargetHotend(t, physical_tool); },
        [](NoTool) { debug_assert(false); });
}

void safe_delay_keep_alive(uint16_t t) {
    //    safe_delay(t);
    //    manage_inactivity(true);
    //    ui.update();
    // shouldn't we call idle() instead? At least the MMU communication can be run even during waiting for temperature
    idle(true);
}

void Enable_E0() {
    enable_E(0);
}

void Disable_E0() {
    disable_E(0);
}

bool all_axes_homed() {
    return marlin_server::all_axes_homed();
}

void FullScreenMsg(const char *pgmS, uint8_t slot) {
}

void enqueue_gcode(const char *gcode) {
    marlin_server::enqueue_gcode(gcode);
}

void marlin_resetE() {
    sync_e_position_to(0);
}

} // namespace MMU2
