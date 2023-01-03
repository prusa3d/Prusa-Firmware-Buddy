// selftest_fan.cpp

#include "selftest_fan.h"
#include "wizard_config.hpp"
#include "fanctl.h"
#include "../../../lib/Marlin/Marlin/src/inc/MarlinConfig.h" //EXTRUDER_AUTO_FAN_TEMPERATURE
#include "marlin_server.h"                                   //marlin_server_get_temp_nozzle()
#include "selftest_log.hpp"
#include "i_selftest.hpp"
#include "algorithm_scale.hpp"

#define FANTEST_STOP_DELAY    2000
#define FANTEST_WAIT_DELAY    2500
#define FANTEST_MEASURE_DELAY 7500

using namespace selftest;
LOG_COMPONENT_REF(Selftest);

CSelftestPart_Fan::CSelftestPart_Fan(IPartHandler &state_machine, const FanConfig_t &config,
    SelftestFan_t &result)
    : state_machine(state_machine)
    , m_config(config)
    , rResult(result)
    , m_StartTime(0)
    , m_EndTime(0)
    , m_SampleSum(0)
    , m_SampleCount(0)
    , m_Step(0) {
    m_config.fanctl.EnterSelftestMode();
}

CSelftestPart_Fan::~CSelftestPart_Fan() {
    m_config.fanctl.ExitSelftestMode();
}

uint32_t CSelftestPart_Fan::estimate(const FanConfig_t &config) {
    uint32_t total_time = FANTEST_STOP_DELAY + config.steps * (FANTEST_WAIT_DELAY + FANTEST_MEASURE_DELAY);
    return total_time;
}

LoopResult CSelftestPart_Fan::stateStart() {
    log_info(Selftest, "%s Started", m_config.partname);
    rResult.state = SelftestSubtestState_t::running;
    SelftestInstance().log_printf("%s Started\n", m_config.partname);
    m_StartTime = SelftestInstance().GetTime();
    m_EndTime = m_StartTime + estimate(m_config);
    if ((m_config.fanctl.getPWM() == 0) && (m_config.fanctl.getActualRPM() == 0)) {
        m_EndTime -= FANTEST_STOP_DELAY; // no need to wait until stop, already stopped
    }
    m_config.fanctl.SelftestSetPWM(0);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::stateWaitStopped() {
    if (state_machine.IsInState_ms() <= FANTEST_STOP_DELAY) {
        actualizeProgress();
        return LoopResult::RunCurrent;
    }
    m_config.fanctl.SelftestSetPWM(m_config.pwm_start);
    m_Step = 0;
    log_info(Selftest, "%s wait stopped, rpm: %d pwm: %d", m_config.partname, m_config.fanctl.getActualRPM(), m_config.fanctl.getPWM());
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::stateWaitRpm() {
    if (state_machine.IsInState_ms() <= FANTEST_WAIT_DELAY) { //wait
        actualizeProgress();
        return LoopResult::RunCurrent;
    }
    m_SampleCount = 0;
    m_SampleSum = 0;
    log_info(Selftest, "%s rpm: %d pwm: %d", m_config.partname, m_config.fanctl.getActualRPM(), m_config.fanctl.getPWM());
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::stateMeasureRpm() {
    if (state_machine.IsInState_ms() <= FANTEST_MEASURE_DELAY) {
        m_SampleCount++;
        m_SampleSum += m_config.fanctl.getActualRPM();
        actualizeProgress();
        return LoopResult::RunCurrent;
    }
    uint16_t rpm = m_SampleSum / m_SampleCount;
    SelftestInstance().log_printf("%s at %u%% PWM = %u RPM\n", m_config.partname, 2 * (m_config.fanctl.getPWM()), rpm);
    if ((m_config.rpm_min_table != nullptr) && (m_config.rpm_max_table != nullptr))
        if ((rpm < m_config.rpm_min_table[m_Step]) || (rpm > m_config.rpm_max_table[m_Step])) {
            SelftestInstance().log_printf("%s %u RPM out of range (%u - %u)\n", m_config.partname, rpm, m_config.rpm_min_table[m_Step], m_config.rpm_max_table[m_Step]);
            log_error(Selftest, "%s measure rpm, rpm: %d pwm: %d", m_config.partname, m_config.fanctl.getActualRPM(), m_config.fanctl.getPWM());
            return LoopResult::Fail;
        }
    if (++m_Step < m_config.steps) {
        m_config.fanctl.SelftestSetPWM(m_config.fanctl.getPWM() + m_config.pwm_step);
        return LoopResult::GoToMark;
    }
    //finish
    log_info(Selftest, "%s Finished\n", m_config.partname);
    return LoopResult::RunNext;
}

void CSelftestPart_Fan::actualizeProgress() const {
    if (m_StartTime == m_EndTime)
        return; // don't have estimated end set correctly
    rResult.progress = scale_percent(SelftestInstance().GetTime(), m_StartTime, m_EndTime);
}
