// selftest_fan.cpp

#include "selftest_fan.h"
#include "wizard_config.hpp"
#include "fanctl.hpp"
#include "config_features.h" //EXTRUDER_AUTO_FAN_TEMPERATURE
#include "marlin_server.hpp" //marlin_server::get_temp_nozzle()
#include "selftest_log.hpp"
#include "i_selftest.hpp"
#include "algorithm_scale.hpp"
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include "module/prusa/toolchanger.h"
#endif

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
    m_config.fanctl_fnc(m_config.tool_nr).EnterSelftestMode();
}

CSelftestPart_Fan::~CSelftestPart_Fan() {
    m_config.fanctl_fnc(m_config.tool_nr).ExitSelftestMode();
}

uint32_t CSelftestPart_Fan::estimate(const FanConfig_t &config) {
    uint32_t total_time = FANTEST_STOP_DELAY + config.steps * (FANTEST_WAIT_DELAY + FANTEST_MEASURE_DELAY);
    return total_time;
}

LoopResult CSelftestPart_Fan::stateStart() {
#if HAS_TOOLCHANGER()
    if (!prusa_toolchanger.is_tool_enabled(m_config.tool_nr)) {
        m_StartTime = m_EndTime = SelftestInstance().GetTime();
        rResult.state = SelftestSubtestState_t::undef;
        return LoopResult::Abort;
    }
#endif

    log_info(Selftest, "%s %d Started", get_partname(), m_config.tool_nr);
    rResult.state = SelftestSubtestState_t::running;
    SelftestInstance().log_printf("%s %d Started\n", get_partname(), m_config.tool_nr);
    m_StartTime = SelftestInstance().GetTime();
    m_EndTime = m_StartTime + estimate(m_config);
    if ((m_config.fanctl_fnc(m_config.tool_nr).getPWM() == 0) && (m_config.fanctl_fnc(m_config.tool_nr).getActualRPM() == 0)) {
        m_EndTime -= FANTEST_STOP_DELAY; // no need to wait until stop, already stopped
    }
    m_config.fanctl_fnc(m_config.tool_nr).SelftestSetPWM(0);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::stateWaitStopped() {
    if (state_machine.IsInState_ms() <= FANTEST_STOP_DELAY) {
        actualizeProgress();
        return LoopResult::RunCurrent;
    }
    m_config.fanctl_fnc(m_config.tool_nr).SelftestSetPWM(m_config.pwm_start);
    m_Step = 0;
    log_info(Selftest, "%s %d wait stopped, rpm: %d pwm: %d", get_partname(), m_config.tool_nr, m_config.fanctl_fnc(m_config.tool_nr).getActualRPM(), m_config.fanctl_fnc(m_config.tool_nr).getPWM());
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::stateWaitRpm() {
    if (state_machine.IsInState_ms() <= FANTEST_WAIT_DELAY) { //wait
        actualizeProgress();
        return LoopResult::RunCurrent;
    }
    m_SampleCount = 0;
    m_SampleSum = 0;
    log_info(Selftest, "%s %d rpm: %d pwm: %d", get_partname(), m_config.tool_nr, m_config.fanctl_fnc(m_config.tool_nr).getActualRPM(), m_config.fanctl_fnc(m_config.tool_nr).getPWM());
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Fan::stateMeasureRpm() {
    if (state_machine.IsInState_ms() <= FANTEST_MEASURE_DELAY) {
        m_SampleCount++;
        m_SampleSum += m_config.fanctl_fnc(m_config.tool_nr).getActualRPM();
        actualizeProgress();
        return LoopResult::RunCurrent;
    }
    uint16_t rpm = m_SampleSum / m_SampleCount;
    SelftestInstance().log_printf("%s %d at %u%% PWM = %u RPM\n", get_partname(), m_config.tool_nr, 2 * (m_config.fanctl_fnc(m_config.tool_nr).getPWM()), rpm);
    if ((m_config.rpm_min_table != nullptr) && (m_config.rpm_max_table != nullptr))
        if ((rpm < m_config.rpm_min_table[m_Step]) || (rpm > m_config.rpm_max_table[m_Step])) {
            SelftestInstance().log_printf("%s %d %u RPM out of range (%u - %u)\n", get_partname(), m_config.tool_nr, rpm, m_config.rpm_min_table[m_Step], m_config.rpm_max_table[m_Step]);
            log_error(Selftest, "%s %d measure rpm, rpm: %d pwm: %d", get_partname(), m_config.tool_nr, m_config.fanctl_fnc(m_config.tool_nr).getActualRPM(), m_config.fanctl_fnc(m_config.tool_nr).getPWM());
            return LoopResult::Fail;
        }
    if (++m_Step < m_config.steps) {
        m_config.fanctl_fnc(m_config.tool_nr).SelftestSetPWM(m_config.fanctl_fnc(m_config.tool_nr).getPWM() + m_config.pwm_step);
        return LoopResult::GoToMark;
    }
    //finish
    log_info(Selftest, "%s %d Finished\n", get_partname(), m_config.tool_nr);
    return LoopResult::RunNext;
}

void CSelftestPart_Fan::actualizeProgress() const {
    if (m_StartTime == m_EndTime)
        return; // don't have estimated end set correctly
    rResult.progress = scale_percent(SelftestInstance().GetTime(), m_StartTime, m_EndTime);
}

const char *CSelftestPart_Fan::get_partname() {
    return (m_config.type == fan_type_t::Heatbreak) ? "Heatbreak fan" : "Print fan";
}
