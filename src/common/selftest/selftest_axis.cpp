// selftest_axis.cpp

#include "selftest_axis.h"
#include "../../Marlin/src/module/planner.h"
#include "../../Marlin/src/module/stepper.h"
#include "../../Marlin/src/module/endstops.h"

static const char AxisLetter[] = { 'X', 'Y', 'Z', 'E' };

CSelftestPart_Axis::CSelftestPart_Axis(const selftest_axis_config_t *pconfig)
    : m_State(spsStart)
    , m_pConfig(pconfig) {
}

bool CSelftestPart_Axis::Start() {
    return true;
}

bool CSelftestPart_Axis::IsInProgress() const {
    return ((m_State != spsIdle) && (m_State != spsFinished) && (m_State != spsAborted));
}

bool CSelftestPart_Axis::Loop() {
    switch (m_State) {
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
        Selftest.log_printf("%s Finished\n", m_pConfig->partname);
        break;
    case spsFinished:
    case spsAborted:
        return false;
    }
    return next();
}

bool CSelftestPart_Axis::Abort() {
    return true;
}

void CSelftestPart_Axis::phaseMove(int8_t dir) {
    Selftest.log_printf("%s fwd @%d mm/s\n", m_pConfig->partname, (int)m_pConfig->fr_table[m_Step]);
    planner.synchronize();
    m_StartPos_usteps = stepper.position((AxisEnum)m_pConfig->axis);
    current_position.pos[m_pConfig->axis] += dir * (m_pConfig->length + 10);
    line_to_current_position(m_pConfig->fr_table[m_Step]);
}

bool CSelftestPart_Axis::phaseWait(int8_t dir) {
    if (planner.movesplanned())
        return true;
    int32_t endPos_usteps = stepper.position((AxisEnum)m_pConfig->axis);
    int32_t length_usteps = dir * (endPos_usteps - m_StartPos_usteps);
    Selftest.log_printf(" length = %f\n", (double)length_usteps / 100);
    return false;
}

bool CSelftestPart_Axis::next() {
    if ((m_State == spsFinished) || (m_State == spsAborted))
        return false;
    m_State = (TestState)((int)m_State + 1);
    return ((m_State != spsFinished) && (m_State != spsAborted));
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
