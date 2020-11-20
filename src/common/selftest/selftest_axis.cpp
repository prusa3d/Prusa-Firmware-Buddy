// selftest_axis.cpp

#include "selftest_axis.h"
#include "wizard_config.hpp"
#include "../../Marlin/src/module/planner.h"
#include "../../Marlin/src/module/stepper.h"
#include "../../Marlin/src/module/endstops.h"
#include "trinamic.h"

static const char AxisLetter[] = { 'X', 'Y', 'Z', 'E' };

CSelftestPart_Axis::CSelftestPart_Axis(const selftest_axis_config_t *pconfig)
    : m_pConfig(pconfig) {
    m_State = spsStart;
}

bool CSelftestPart_Axis::Start() {
    if (!IsInProgress()) {
        m_State = spsStart;
        return true;
    }
    return false;
}

bool CSelftestPart_Axis::IsInProgress() const {
    return ((m_State != spsIdle) && (m_State < spsFinished));
}

bool CSelftestPart_Axis::Loop() {
    switch ((TestState)m_State) {
    case spsIdle:
        return false;
    case spsStart: {
        Selftest.log_printf("%s Started\n", m_pConfig->partname);
        m_Time = Selftest.m_Time;
        m_StartTime = m_Time;
        m_EndTime = m_StartTime + estimate(m_pConfig);
        char gcode[6];
        sprintf(gcode, "G28 %c", AxisLetter[m_pConfig->axis]);
        queue.enqueue_one_now(gcode);
        break;
    }
    case spsWaitHome:
        if (planner.movesplanned() || queue.length)
            return true;
        endstops.enable(true);
        endstops.enable_z_probe();
        m_Step = 0;
        m_Time = Selftest.m_Time;
        break;
    case spsMoveFwd:
        phaseMove(m_pConfig->dir);
        break;
    case spsWaitFwd:
        if (phaseWait(m_pConfig->dir))
            return true;
        break;
    case spsMoveRev:
        phaseMove(-m_pConfig->dir);
        break;
    case spsWaitRev:
        if (phaseWait(-m_pConfig->dir))
            return true;
        if (++m_Step < m_pConfig->steps) {
            m_State = spsMoveFwd;
            return true;
        }
        break;
    case spsFinish:
        endstops.enable(false);
        if (m_Result == sprFailed)
            m_State = spsFailed;
        else
            m_Result = sprPassed;
        Selftest.log_printf("%s %s\n", m_pConfig->partname, (m_Result == sprPassed) ? "Passed" : "Failed");
        break;
    case spsFinished:
    case spsAborted:
    case spsFailed:
        return false;
    }
    return next();
}

bool CSelftestPart_Axis::Abort() {
    return true;
}

uint8_t CSelftestPart_Axis::getFSMState() {
    if (m_State <= spsStart)
        return (uint8_t)(SelftestSubtestState_t::undef);
    else if (m_State < spsFinished)
        return (uint8_t)(SelftestSubtestState_t::running);
    return (uint8_t)((m_Result == sprPassed) ? (SelftestSubtestState_t::ok) : (SelftestSubtestState_t::not_good));
}

void CSelftestPart_Axis::phaseMove(int8_t dir) {
    Selftest.log_printf("%s %s @%d mm/s\n", ((dir * m_pConfig->dir) > 0) ? "fwd" : "rew", m_pConfig->partname, (int)m_pConfig->fr_table[m_Step]);
    planner.synchronize();
    sg_sampling_enable();
    m_StartPos_usteps = stepper.position((AxisEnum)m_pConfig->axis);
    current_position.pos[m_pConfig->axis] += dir * (m_pConfig->length + 10);
    line_to_current_position(m_pConfig->fr_table[m_Step]);
}

bool CSelftestPart_Axis::phaseWait(int8_t dir) {
    if (planner.movesplanned())
        return true;
    sg_sampling_disable();
    int32_t endPos_usteps = stepper.position((AxisEnum)m_pConfig->axis);
    int32_t length_usteps = dir * (endPos_usteps - m_StartPos_usteps);
    float length_mm = (length_usteps * planner.steps_to_mm[(AxisEnum)m_pConfig->axis]);
    Selftest.log_printf(" length = %f mm\n", (double)length_mm);
    if ((length_mm < m_pConfig->length_min) || (length_mm > m_pConfig->length_max)) {
        m_Result = sprFailed;
        m_State = spsFinish;
        return true;
    }
    return false;
}

uint32_t CSelftestPart_Axis::estimate(const selftest_axis_config_t *pconfig) {
    uint32_t total_time = 0;
    for (int i = 0; i < pconfig->steps; i++) {
        total_time += 2 * estimate_move(pconfig->length, pconfig->fr_table[i]);
    }
    return total_time;
}

uint32_t CSelftestPart_Axis::estimate_move(float len_mm, float fr_mms) {
    uint32_t move_time = 1000 * len_mm / fr_mms;
    return move_time;
}

void CSelftestPart_Axis::sg_sample_cb(uint8_t axis, uint16_t sg) {
    if (m_pSGAxis && (m_pSGAxis->m_pConfig->axis == axis))
        m_pSGAxis->sg_sample(sg);
}

void CSelftestPart_Axis::sg_sample(uint16_t sg) {
    int32_t pos = stepper.position((AxisEnum)m_pConfig->axis);
    Selftest.log_printf("%u %d %d\n", HAL_GetTick() - m_StartTime, pos, sg);
    m_SGCount++;
    m_SGSum += sg;
}

void CSelftestPart_Axis::sg_sampling_enable() {
    m_SGOrig_mask = tmc_get_sg_mask();
    tmc_set_sg_mask(1 << m_pConfig->axis);
    tmc_set_sg_axis(m_pConfig->axis);
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
