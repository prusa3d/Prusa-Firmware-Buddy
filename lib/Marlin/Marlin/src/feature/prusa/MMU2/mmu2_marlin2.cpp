/// @file
/// Marlin2 interface implementation for the MMU2
#include "mmu2_marlin.h"

#include "mapi/motion.hpp"

#include "../../inc/MarlinConfig.h"
#include "../../gcode/gcode.h"
#include "../../libs/buzzer.h"
#include "../../libs/nozzle.h"
#include "../../module/temperature.h"
#include "../../module/planner.h"
#include "../../module/stepper/indirection.h"
#include "../../Marlin.h"
#include "marlin_server.hpp"

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
    bool emptying_buffer_orig = planner.emptying();
    planner.set_emptying_buffer(true);
    while (planner.busy()) {
        idle(true);
        udd();
    }
    planner.set_emptying_buffer(emptying_buffer_orig);
}

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
    xyze_pos_t target_pos = current_position;
    target_pos.z = z;

#if HAS_LEVELING && !PLANNER_LEVELING
    // Gotta apply leveling, otherwise the move would move to non-leveled coordinates
    // (and potentially crash into model)
    // If PLANNER_LEVELING is true, the leveling is applied inside buffer_line
    planner.apply_leveling(target_pos);
#endif

    do_blocking_move_to(target_pos, feedRate_mm_s);
}

void nozzle_park() {
    static constexpr xyz_pos_t park_point = NOZZLE_PARK_POINT_M600;
    nozzle.park(2, park_point);
}

bool marlin_printingIsActive() {
    return printingIsActive();
}

void marlin_manage_heater() {
    thermalManager.manage_heater();
}

void marlin_manage_inactivity(bool ignore_stepper_queue) {
    manage_inactivity(ignore_stepper_queue);
}

void marlin_idle(bool waiting, bool ignore_stepper_queue) {
    idle(waiting, ignore_stepper_queue);
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

int16_t thermal_degTargetHotend() {
    return thermalManager.degTargetHotend(active_extruder);
}

int16_t thermal_degHotend() {
    return thermalManager.degHotend(active_extruder);
}

void thermal_setExtrudeMintemp(int16_t t) {
    thermalManager.extrude_min_temp = t;
}

void thermal_setTargetHotend(int16_t t) {
    thermalManager.setTargetHotend(t, active_extruder);
}

void safe_delay_keep_alive(uint16_t t) {
    //    safe_delay(t);
    //    manage_inactivity(true);
    //    ui.update();
    // shouldn't we call idle() instead? At least the MMU communication can be run even during waiting for temperature
    idle(true, true);
}

void Enable_E0() {
    enable_E0();
}

void Disable_E0() {
    disable_E0();
}

bool all_axes_homed() {
    return marlin_server::all_axes_homed();
}

void FullScreenMsg(const char *pgmS, uint8_t slot) {
}

void enqueue_gcode(const char *gcode) {
    marlin_server::enqueue_gcode(gcode);
}

} // namespace MMU2
