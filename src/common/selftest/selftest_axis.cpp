// selftest_axis.cpp

#include "selftest_axis.h"
#include "../../Marlin/src/module/planner.h"
#include "../../Marlin/src/module/stepper.h"
#include "../../Marlin/src/module/endstops.h"
#include "../../Marlin/src/module/motion.h"
#include "../../Marlin/src/module/prusa/homing_utils.hpp"
#include "trinamic.h"
#include "selftest_log.hpp"
#include "i_selftest.hpp"
#include "algorithm_scale.hpp"
#include "printers.h"
#include "homing_reporter.hpp"
#include "config_store/store_instance.hpp"

#include <limits>
#include <option/has_loadcell.h>
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif /*HAS_TOOLCHANGER()*/

using namespace selftest;
LOG_COMPONENT_REF(Selftest);

CSelftestPart_Axis::CSelftestPart_Axis(IPartHandler &state_machine, const AxisConfig_t &config,
    SelftestSingleAxis_t &result)
    : state_machine(state_machine)
    , config(config)
    , rResult(result)
    , log(1000) {
    log_info(Selftest, "%s Started", config.partname);
    homing_reset();
}

CSelftestPart_Axis::~CSelftestPart_Axis() {
    endstops.enable(false);
    endstops.enable_z_probe(false);
}

void CSelftestPart_Axis::phaseMove(int8_t dir) {
    const float feedrate = dir > 0 ? config.fr_table_fw[m_Step / 2] : config.fr_table_bw[m_Step / 2];
    log_info(Selftest, "%s %s @%d mm/s", ((dir * config.movement_dir) > 0) ? "fwd" : "rew", config.partname, int(feedrate));
    planner.synchronize();
    endstops.validate_homing_move();
    sg_sampling_enable();

    set_current_from_steppers();
    sync_plan_position();
    report_current_position();

    m_StartPos_usteps = stepper.position((AxisEnum)config.axis);

    // Disable stealthChop if used. Enable diag1 pin on driver.
#if ENABLED(SENSORLESS_HOMING)
    #if HOMING_Z_WITH_PROBE && HAS_LOADCELL()
    const bool is_home_dir = (home_dir(AxisEnum(config.axis)) > 0) == (dir > 0);
    const bool moving_probe_toward_bed = (is_home_dir && (Z_AXIS == config.axis));
    #endif
    bool enable_sensorless_homing =
    #if HOMING_Z_WITH_PROBE && HAS_LOADCELL()
        !moving_probe_toward_bed
    #else
        true
    #endif
        ;

    if (enable_sensorless_homing) {
        start_sensorless_homing_per_axis(AxisEnum(config.axis));
    }
#endif
#if !PRINTER_IS_PRUSA_XL()
    // Unmeasured distance is to check the exact length of an axis. Since XL doesn't check full axis and only print area, this measurement is therefore obsolete
    unmeasured_distance = std::abs((dir > 0 ? soft_endstop.min : soft_endstop.max)[config.axis] - current_position.pos[config.axis]);
#endif

    current_position.pos[config.axis] += dir * (config.length + EXTRA_LEN_MM);
    line_to_current_position(feedrate);
}

LoopResult CSelftestPart_Axis::wait(int8_t dir) {
    actualizeProgress();
    if (planner.processing()) {
        return LoopResult::RunCurrent;
    }
    sg_sampling_disable();

    set_current_from_steppers();
    sync_plan_position();
    report_current_position();

#if HAS_HOTEND_OFFSET
    // Tool offset was just trashed, moreover this home was not precise
    // Force home on next toolchange
    CBI(axis_known_position, X_AXIS);
    CBI(axis_known_position, Y_AXIS);
#endif

    int32_t endPos_usteps = stepper.position((AxisEnum)config.axis);
    int32_t length_usteps = dir * (endPos_usteps - m_StartPos_usteps);
    float length_mm = (length_usteps * planner.mm_per_step[(AxisEnum)config.axis]);

// Core kinematic has inverted Y steps compared to axis move direction
#if CORE_IS_XY || CORE_IS_YZ
    if (static_cast<AxisEnum>(config.axis) == AxisEnum::Y_AXIS) {
        length_mm *= -1;
    }
#endif
#if !PRINTER_IS_PRUSA_XL()
    length_mm += unmeasured_distance;
#endif

    if ((length_mm < config.length_min) || (length_mm > config.length_max)) {
        log_error(Selftest, "%s measured length = %fmm out of range <%f,%f>", config.partname, (double)length_mm, (double)config.length_min, (double)config.length_max);
        return LoopResult::Fail;
    }
    log_info(Selftest, "%s measured length = %fmm out in range <%f,%f>", config.partname, (double)length_mm, (double)config.length_min, (double)config.length_max);
    return LoopResult::RunNext;
}

uint32_t CSelftestPart_Axis::estimate(const AxisConfig_t &config) {
    uint32_t total_time = 0;
    for (uint32_t i = 0; i < config.steps; i++) {
        total_time += estimate_move(config.length, (config.steps % 2) ? config.fr_table_bw[i] : config.fr_table_fw[i]);
    }
    return total_time;
}

uint32_t CSelftestPart_Axis::estimate_move(float len_mm, float fr_mms) {
    uint32_t move_time = 1000 * len_mm / fr_mms;
    return move_time;
}

void CSelftestPart_Axis::sg_sample_cb(uint8_t axis, uint16_t sg) {
    if (m_pSGAxis && (m_pSGAxis->config.axis == axis)) {
        m_pSGAxis->sg_sample(sg);
    }
}

void CSelftestPart_Axis::sg_sample(uint16_t sg) {
    [[maybe_unused]] int32_t pos = stepper.position((AxisEnum)config.axis);
    LogDebugTimed(log, "%s time %" PRIu32 "ms pos: %" PRId32 " sg: %" PRIu16,
        config.partname, static_cast<uint32_t>(SelftestInstance().GetTime() - time_progress_start), pos, sg);
    m_SGCount++;
    m_SGSum += sg;
}

void CSelftestPart_Axis::sg_sampling_enable() {
    m_SGOrig_mask = tmc_get_sg_mask();
    tmc_set_sg_mask(1 << config.axis);
    tmc_set_sg_axis(config.axis);
    m_pSGOrig_cb = (void *)tmc_get_sg_sample_cb();
    tmc_set_sg_sample_cb(sg_sample_cb);
    m_pSGAxis = this;
    m_SGCount = 0;
    m_SGSum = 0;
}

void CSelftestPart_Axis::sg_sampling_disable() {
    tmc_set_sg_mask(m_SGOrig_mask);
    tmc_set_sg_axis(0);
    tmc_set_sg_sample_cb((tmc_sg_sample_cb_t *)m_pSGOrig_cb);
    m_pSGAxis = nullptr;
}

CSelftestPart_Axis *CSelftestPart_Axis::m_pSGAxis = nullptr;

LoopResult CSelftestPart_Axis::stateActivateHomingReporter() {
    HomingReporter::enable();
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Axis::stateHomeXY() {
    // Mark axis as not homed in case it was marked as homed before
    set_axis_is_not_at_home(AxisEnum(config.axis));

    // Trigger home on axis
    ArrayStringBuilder<10> sb;
#if PRINTER_IS_PRUSA_MK4()
    // MK4 needs to be able to calibrate homing here, i.e.
    // not have "D"o not calibrate parameter set
    // (yes, those double negatives are fun).
    sb.append_printf("G28 %c P", iaxis_codes[config.axis]);
#else
    sb.append_printf("G28 %c D P", iaxis_codes[config.axis]);
#endif
    queue.enqueue_one_now(sb.str());

    log_info(Selftest, "%s home single axis", config.partname);

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Axis::stateWaitHomingReporter() {
    HomingReporter::State state = HomingReporter::consume_done();

    switch (state) {
    case HomingReporter::State::disabled:
    case HomingReporter::State::enabled:
        log_error(Selftest, "%s homing reporter is in wrong state", config.partname);
        [[fallthrough]];
    case HomingReporter::State::done:
        return LoopResult::RunNext;
    case HomingReporter::State::in_progress:
        break;
    }
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_Axis::stateEvaluateHomingXY() {
    if (axes_need_homing(_BV(config.axis))) {
        return LoopResult::Fail;
    }

    endstops.enable(true);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Axis::stateHomeZ() {
    // we have Z safe homing enabled, so Z might need to home all axis
    if (!TEST(axis_known_position, X_AXIS) || !TEST(axis_known_position, Y_AXIS)) {
        log_info(Selftest, "%s home all axis", config.partname);
        queue.enqueue_one_now("G28 D P");
    } else {
        log_info(Selftest, "%s home single axis", config.partname);
        queue.enqueue_one_now("G28 Z P");
    }

#if HAS_TOOLCHANGER()
    // Z axis check needs to be done with a tool
    if (prusa_toolchanger.is_toolchanger_enabled() && (prusa_toolchanger.has_tool() == false)) {
        queue.enqueue_one_now("T0 S1");
    }
#endif /*HAS_TOOLCHANGER()*/

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Axis::stateWaitHome() {
    if (queue.has_commands_queued() || planner.processing()) {
        return LoopResult::RunCurrent;
    }
    endstops.enable(true);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Axis::stateEnableZProbe() {
    endstops.enable_z_probe();
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Axis::stateInitProgressTimeCalculation() {
    time_progress_start = SelftestInstance().GetTime();
    time_progress_estimated_end = time_progress_start + estimate(config);
    rResult.state = SelftestSubtestState_t::running;
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Axis::stateMove() {
    phaseMove(getDir());
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Axis::stateMoveFinishCycle() {
    check_coils();

    LoopResult result = wait(getDir());
    if (result == LoopResult::Fail) {
        // The test failed - this also leaves the printbed parked over
        // the screen. We need to move the heatbed back a little so the
        // screen is visible. Then we can fail the test.
        return LoopResult::RunNextAndFailAfter;
    }
    if (result != LoopResult::RunNext) {
        return result;
    }
    if ((++m_Step) < config.steps) {
        return LoopResult::GoToMark2;
    }
    return LoopResult::RunNext;
}

// MK4 heatbed stays in the front after Y axis selftest, blocking display view
// XL continues with homing which can be loud if starting at the edge
// Move Y to better position after selftest is completed
LoopResult CSelftestPart_Axis::stateParkAxis() {
    check_coils();

    static bool parking_initiated = false;
    if (queue.has_commands_queued() || planner.processing()) {
        return LoopResult::RunCurrent;
    }
    if (parking_initiated) {
        parking_initiated = false;
        return LoopResult::RunNext;
    }

    if (config.park) {
        char gcode[15];
        endstops.enable(false);
        log_info(Selftest, "%s park %c axis to %i", config.partname, iaxis_codes[config.axis], static_cast<int>(config.park_pos));
        snprintf(gcode, std::size(gcode), "G1 %c%i F4200", iaxis_codes[config.axis], static_cast<int>(config.park_pos));
        queue.enqueue_one_now(gcode); // Park Y
        parking_initiated = true;
        return LoopResult::RunCurrent;
    } else {
        return LoopResult::RunNext;
    }
}

void CSelftestPart_Axis::actualizeProgress() const {
    if (time_progress_start == time_progress_estimated_end) {
        return; // don't have estimated end set correctly
    }
    rResult.progress = scale_percent_avoid_overflow(SelftestInstance().GetTime(), time_progress_start, time_progress_estimated_end);
}

void CSelftestPart_Axis::check_coils() {
    // Update coil check result. This is not reliable. We are fine with one ok reading

    // On coreXY and when actually testing X,Y axes we need to check both A, B steppers
    // as we would like X axis check to fail even when B stepper is not ok and vice versa.
#ifdef COREXY
    const bool check_ab = config.axis == X_AXIS || config.axis == Y_AXIS;
#else
    const bool check_ab = false;
#endif

    if (check_ab) {
        if (tmc_check_coils(A_AXIS) && tmc_check_coils(B_AXIS)) {
            coils_ok = true;
        }
    } else {
        if (tmc_check_coils(config.axis)) {
            coils_ok = true;
        }
    }
}

LoopResult CSelftestPart_Axis::state_verify_coils() {
    if (!coils_ok) {
        log_error(Selftest, "Axis coil error");
        return LoopResult::Fail;
    }
    return LoopResult::RunNext;
}
