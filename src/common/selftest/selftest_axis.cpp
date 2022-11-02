// selftest_axis.cpp

#include "selftest_axis.h"
#include "wizard_config.hpp"
#include "../../Marlin/src/module/planner.h"
#include "../../Marlin/src/module/stepper.h"
#include "../../Marlin/src/module/endstops.h"
#include "../../Marlin/src/module/motion.h"
#include "../../Marlin/src/feature/prusa/homing.h"
#include "trinamic.h"
#include "selftest_log.hpp"
#include "i_selftest.hpp"
#include "algorithm_scale.hpp"

using namespace selftest;
LOG_COMPONENT_REF(Selftest);

static const char AxisLetter[] = { 'X', 'Y', 'Z', 'E' };
CSelftestPart_Axis::CSelftestPart_Axis(IPartHandler &state_machine, const AxisConfig_t &config,
    SelftestSingleAxis_t &result)
    : state_machine(state_machine)
    , config(config)
    , rResult(result)
    , time_progress_start(0)
    , time_progress_estimated_end(0)
    , m_Step(0)
    , m_StartPos_usteps(0)
    , m_SGCount(0)
    , m_SGSum(0)
    , m_pSGOrig_cb(nullptr)
    , log(1000) {
    log_info(Selftest, "%s Started", config.partname);
    homing_reset();
    char gcode[6];
    // we have Z safe homing enabled, so Z might need to home all axis
    if (AxisLetter[config.axis] == 'Z' && (!axis_is_trusted(X_AXIS) || !axis_is_trusted(Y_AXIS))) {
        log_info(Selftest, "%s home all axis", config.partname);
        sprintf(gcode, "G28");
    } else {
        sprintf(gcode, "G28 %c", AxisLetter[config.axis]);
        log_info(Selftest, "%s home single axis", config.partname);
    }
    queue.enqueue_one_now(gcode);
}

CSelftestPart_Axis::~CSelftestPart_Axis() { endstops.enable(false); }

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
    #if HOMING_Z_WITH_PROBE && 0
    const bool is_home_dir = (home_dir(AxisEnum(config.axis)) > 0) == (dir > 0);
    const bool moving_probe_toward_bed = (is_home_dir && (Z_AXIS == config.axis));
    #endif
    bool enable_sensorless_homing =
    #if HOMING_Z_WITH_PROBE && 0
        !moving_probe_toward_bed
    #else
        true
    #endif
        ;

    if (enable_sensorless_homing)
        start_sensorless_homing_per_axis(AxisEnum(config.axis));
#endif

    current_position.pos[config.axis] += dir * (config.length + 10);
    line_to_current_position(feedrate);
}

LoopResult CSelftestPart_Axis::wait(int8_t dir) {
    actualizeProgress();
    if (planner.movesplanned())
        return LoopResult::RunCurrent;
    sg_sampling_disable();
    int32_t endPos_usteps = stepper.position((AxisEnum)config.axis);
    int32_t length_usteps = dir * (endPos_usteps - m_StartPos_usteps);
    float length_mm = (length_usteps * planner.mm_per_step[(AxisEnum)config.axis]);
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
    if (m_pSGAxis && (m_pSGAxis->config.axis == axis))
        m_pSGAxis->sg_sample(sg);
}

void CSelftestPart_Axis::sg_sample(uint16_t sg) {
    [[maybe_unused]] int32_t pos = stepper.position((AxisEnum)config.axis);
    LogDebugTimed(log, "%s time %ums pos: %d sg: %d", config.partname, SelftestInstance().GetTime() - time_progress_start, pos, sg);
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

LoopResult CSelftestPart_Axis::stateWaitHome() {
    if (planner.movesplanned() || queue.length)
        return LoopResult::RunCurrent;
    endstops.enable(true);
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

LoopResult CSelftestPart_Axis::stateMoveWaitFinish() {
    LoopResult result = wait(getDir());
    if (result != LoopResult::RunNext)
        return result;
    if ((++m_Step) < config.steps) {
        return LoopResult::GoToMark;
    }
    return LoopResult::RunNext;
}

void CSelftestPart_Axis::actualizeProgress() const {
    if (time_progress_start == time_progress_estimated_end)
        return; // don't have estimated end set correctly
    rResult.progress = scale_percent_avoid_overflow(SelftestInstance().GetTime(), time_progress_start, time_progress_estimated_end);
}
