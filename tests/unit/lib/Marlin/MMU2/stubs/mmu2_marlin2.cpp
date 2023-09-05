#include <mmu2_marlin.h>
#include <mmu2_mk4.h>
#include "stub_interfaces.h"

namespace MMU2 {

void extruder_move(float distance, float feed_rate) {
    //    mockLog_RecordFn();
}
void extruder_schedule_turning(float feed_rate) {
    mockLog_RecordFn();
}

float raise_z(float delta) {
    mockLog_RecordFn();
    return 0;
}

// void planner_abort_queued_moves();
void planner_synchronize() {
    //    mockLog_RecordFn();
}
bool planner_any_moves() {
    mockLog_RecordFn();
    return false;
}
pos3d planner_current_position() { return { 0, 0, 0 }; }

void motion_do_blocking_move_to_xy(float rx, float ry, float feedRate_mm_s) {
    mockLog_RecordFn();
}
void motion_do_blocking_move_to_z(float z, float feedRate_mm_s) {
    mockLog_RecordFn();
}
void nozzle_park() {
    mockLog_RecordFn();
}

bool marlin_printingIsActive() { return false; }
void marlin_manage_heater() {
    mockLog_RecordFn();
}
void marlin_manage_inactivity(bool b) {
    mockLog_RecordFn();
}
void marlin_idle() {
    mmu2.mmu_loop();
}

int16_t thermal_degTargetHotend() { return 0; }
int16_t thermal_degHotend() { return 0; }
void thermal_setExtrudeMintemp(int16_t t) {
    mockLog_RecordFn();
}
void thermal_setTargetHotend(int16_t t) {
    mockLog_RecordFn();
}

void safe_delay_keep_alive(uint16_t t) {
    mockLog_RecordFn();
}

void Enable_E0() {
    //    mockLog_RecordFn();
}
void Disable_E0() {
    //    mockLog_RecordFn();
}

bool all_axes_homed() { return false; }

void gcode_reset_stepper_timeout() {
    mockLog_RecordFn();
}

} // namespace MMU2
