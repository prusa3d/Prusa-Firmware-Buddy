#include <mmu2_marlin.h>
#include <mmu2_mk4.h>
#include "stub_interfaces.h"

static bool printingIsActive = false;
void SetMarlinIsPrinting(bool p) {
    printingIsActive = p;
}

static uint16_t hotendTargetTemp = 0;
static uint16_t hotendTemp = 0;

#define mockLog_RecordFnTemp(t) mockLog.Record(std::string { mockLog.MethodName(__PRETTY_FUNCTION__) } + "(" + std::to_string(t) + ")")

void SetHotendTargetTemp(uint16_t t) {
    mockLog_RecordFnTemp(t);
    hotendTargetTemp = t;
}

void SetHotendCurrentTemp(uint16_t t) {
    mockLog_RecordFnTemp(t);
    hotendTemp = t;
}

namespace MMU2 {

void extruder_move(float distance, float feed_rate) {
    //    mockLog_RecordFn();
}
void extruder_schedule_turning(float feed_rate) {
    mockLog_RecordFn();
}

float move_raise_z(float delta) {
    mockLog_RecordFn();
    return 0;
}

// void planner_abort_queued_moves();
void planner_synchronize() {
    //    mockLog_RecordFn();
}

float stepper_get_machine_position_E_mm() {
    return 0;
}

UnloadDistanceDetector::UnloadDistanceDetector() {}
void UnloadDistanceDetector::operator()() {}

void planner_synchronize_hook([[maybe_unused]] UnloadDistanceDetector &udd) {
}

bool planner_any_moves() {
    mockLog_RecordFn();
    return false;
}

uint8_t planner_moves_planned_count() {
    return 0;
}

bool planner_draining() {
    return false;
}

void planner_abort_queued_moves() {}

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

bool marlin_printingIsActive() { return printingIsActive; }
void marlin_manage_heater() {
    mockLog_RecordFn();
}
void marlin_manage_inactivity(bool /*ignore_stepper_queue*/) {
    mockLog_RecordFn();
}
void marlin_idle(bool /*waiting*/, bool /*ignore_stepper_queue*/) {
    mmu2.mmu_loop();
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
}

int16_t thermal_degTargetHotend() {
    return hotendTargetTemp;
}

int16_t thermal_degHotend() {
    return hotendTemp;
}

void thermal_setExtrudeMintemp(int16_t t) {
    mockLog_RecordFnTemp(t);
}

void thermal_setTargetHotend(int16_t t) {
    mockLog_RecordFnTemp(t);
}

void safe_delay_keep_alive(uint16_t t) {
    // normally calls marlin idle() which contains the MMU loop as well
    mmu2.mmu_loop();
}

void Enable_E0() {
    //    mockLog_RecordFn();
}
void Disable_E0() {
    //    mockLog_RecordFn();
}

bool all_axes_homed() { return false; }

void gcode_reset_stepper_timeout() {
    //    mockLog_RecordFn();
}

void enqueue_gcode(const char *gcode) {
}

} // namespace MMU2
