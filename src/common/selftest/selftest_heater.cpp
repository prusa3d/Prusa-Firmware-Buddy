// selftest_heater.cpp

#include "selftest_heater.h"
#include "hwio.h"
#include "selftest_log.hpp"
#include "fanctl.hpp"
#include "../../Marlin/src/module/temperature.h"
#include "i_selftest.hpp"
#include "algorithm_scale.hpp"
#include <option/has_toolchanger.h>
#include "advanced_power.hpp"
#include <printers.h>
#include "config_store/store_instance.hpp"

using namespace selftest;
LOG_COMPONENT_REF(Selftest);

static constexpr float TEMP_DIFF_LIMIT = 0.25;
static constexpr float TEMP_DELTA_LIMIT = 0.05F;
static constexpr uint32_t TEMP_MEASURE_CYCLE_DELAY = 1000;
static constexpr uint32_t TEMP_WAIT_CYCLE_DELAY = 2000;

CSelftestPart_Heater::CSelftestPart_Heater(IPartHandler &state_machine, const HeaterConfig_t &config,
    SelftestHeater_t &result)
    : state_machine(state_machine)
    , m_config(config)
    , rResult(result)
    , storedKp(config.refKp)
    , storedKi(config.refKi)
    , storedKd(config.refKd)
    , last_progress(0)
    , log(2000)
    , check_log(3000) {}

CSelftestPart_Heater::~CSelftestPart_Heater() {
    log_info(Selftest, "%s finish, target: %d current: %f", m_config.partname,
        static_cast<int>(m_config.target_temp), static_cast<double>(m_config.getTemp()));
    m_config.setTargetTemp(0);

    m_config.refKp = storedKp;
    m_config.refKi = storedKi;
    m_config.refKd = storedKd;
    thermalManager.updatePID();
    log_info(Selftest, "%s heater PID regulator restored", m_config.partname);
}

uint32_t CSelftestPart_Heater::estimate(const HeaterConfig_t &config) {
    return config.heat_time_ms;
}

LoopResult CSelftestPart_Heater::stateCheckHbrPassed() {
    SelftestResult eeres = config_store().selftest_result.get();
    if (!eeres.tools[m_config.tool_nr].has_heatbreak_fan_passed()) {
        IPartHandler::SetFsmPhase(PhasesSelftest::HeatersDisabledDialog);
        nozzle_test_skipped = true;
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateShowSkippedDialog() {
    if (!nozzle_test_skipped) {
        return LoopResult::RunNext;
    }

#if HAS_TOOLCHANGER()
    if (prusa_toolchanger.get_num_enabled_tools() > 1) {
        //  We can skip this dialog and always show info text, because toolchanger multitool runs heater tests separately
        return LoopResult::Abort;
    }
#endif

    if (state_machine.GetButtonPressed() == Response::Ok) {
        return LoopResult::Abort;
    }

    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_Heater::stateSetup() {
#if HAS_TOOLCHANGER()
    // if this tool is not enabled, end this test immediately and set result to undefined
    if (!prusa_toolchanger.is_tool_enabled(m_config.tool_nr)) {
        m_StartTime = m_EndTime = SelftestInstance().GetTime();
        rResult.prep_state = SelftestSubtestState_t::undef;
        rResult.heat_state = SelftestSubtestState_t::undef;
        return LoopResult::Abort;
    }
#endif

#if HAS_TOOLCHANGER()
    if (prusa_toolchanger.get_num_enabled_tools() <= 1)
#endif
    {
        // do this for singletool configurations, multitool has special handling
        IPartHandler::SetFsmPhase(PhasesSelftest::Heaters);
    }

    // looked into marlin and it seems all PID values are used as numerator
    // switch regulator into on/off mode
    m_config.refKp = 1000000;
    m_config.refKi = 0;
    m_config.refKd = 0;
    thermalManager.updatePID();
    log_info(Selftest, "%s Started", m_config.partname);
    log_info(Selftest, "%s target: %d current: %f", m_config.partname,
        static_cast<int>(m_config.target_temp), static_cast<double>(m_config.getTemp()));
    log_info(Selftest, "%s heater PID regulator changed to P regulator", m_config.partname);

    m_StartTime = SelftestInstance().GetTime();
    m_EndTime = m_StartTime + estimate(m_config);
    // m_TempDiffSum = 0;
    // m_TempDiffSum = 0;
    // m_TempCount = 0;
    begin_temp = m_config.getTemp();
    enable_cooldown = m_config.getTemp() >= m_config.start_temp;
    m_config.setTargetTemp(0);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateTakeControlOverFans() {
    log_info(Selftest, "%s took control of fans", m_config.partname);
    m_config.print_fan_fnc(m_config.tool_nr).enterSelftestMode();
    m_config.heatbreak_fan_fnc(m_config.tool_nr).enterSelftestMode();
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateFansActivate() {
    if (enable_cooldown) {
        log_info(Selftest, "%s set fans to maximum", m_config.partname);
        m_config.print_fan_fnc(m_config.tool_nr).selftestSetPWM(255); // it will be restored by exitSelftestMode
        m_config.heatbreak_fan_fnc(m_config.tool_nr).selftestSetPWM(255); // it will be restored by exitSelftestMode
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateCooldownInit() {
    if (enable_cooldown) {
        rResult.prep_state = SelftestSubtestState_t::running;
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateCooldown() {
    if (!enable_cooldown) {
        log_info(Selftest, "%s cooldown not needed, target: %d current: %f", m_config.partname,
            static_cast<int>(m_config.target_temp), static_cast<double>(m_config.getTemp()));
        return LoopResult::RunNext;
    }

    LogInfoTimed(log, "%s cooling down, target: %d current: %f", m_config.partname,
        static_cast<int>(m_config.target_temp), static_cast<double>(m_config.getTemp()));

    if (m_config.getTemp() > m_config.undercool_temp) {
        // m_config.undercool_temp .. 100%
        // begin_temp              ..   0%
        actualizeProgress(m_config.getTemp(), begin_temp, m_config.undercool_temp);
        return LoopResult::RunCurrent;
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateFansDeactivate() {
    m_config.print_fan_fnc(m_config.tool_nr).exitSelftestMode();
    m_config.heatbreak_fan_fnc(m_config.tool_nr).exitSelftestMode();
    log_info(Selftest, "%s returned control of fans", m_config.partname);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateTargetTemp() {
    log_info(Selftest, "%s set target, target: %d current: %f", m_config.partname,
        static_cast<int>(m_config.target_temp), static_cast<double>(m_config.getTemp()));
    rResult.prep_state = SelftestSubtestState_t::running; // waiting for preheat temperature
    m_config.setTargetTemp(m_config.target_temp);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateWait() {
    float current_temp = m_config.getTemp();
    if (current_temp >= m_config.start_temp) {
        rResult.prep_state = SelftestSubtestState_t::ok; // preheat temperature ok
        rResult.heat_state = SelftestSubtestState_t::running; // waiting final heat
        m_MeasureStartTime = SelftestInstance().GetTime();
        m_StartTime = SelftestInstance().GetTime();
        m_EndTime = m_StartTime + estimate(m_config);
        rResult.progress = 0;
        log_info(Selftest, "%s wait start temp reached: target: %d current: %f", m_config.partname,
            static_cast<int>(m_config.target_temp), static_cast<double>(m_config.getTemp()));
        return LoopResult::RunNext;
    }
    LogInfoTimed(log, "%s wait, run: target: %d current: %f", m_config.partname,
        static_cast<int>(m_config.target_temp), static_cast<double>(current_temp));

    // m_config.start_temp     .. 100%
    // m_config.undercool_temp ..   0%
    actualizeProgress(current_temp, m_config.undercool_temp, m_config.start_temp);
    return LoopResult::RunCurrent;

#if (0)
    // used to be commented code I just moved it and wrapped in #if (0) instead
    if ((Selftest.m_Time - m_Time) < TEMP_WAIT_CYCLE_DELAY) {
        float temp = m_config.getTemp();
        float temp_diff = (temp - m_config.start_temp);
        float temp_delta = (temp - m_Temp);
        m_Temp = temp;
        m_TempDiffSum += temp_diff;
        m_TempDeltaSum += temp_delta;
        m_TempCount++;
        return true;
    }
    m_TempDiffSum /= m_TempCount;
    m_TempDeltaSum /= m_TempCount;
    if ((fabsf(m_TempDiffSum) > TEMP_DIFF_LIMIT) || (fabsf(m_TempDeltaSum) > TEMP_DELTA_LIMIT)) {
        m_Time = Selftest.m_Time;
        m_TempDiffSum = 0;
        m_TempDeltaSum = 0;
        m_TempCount = 0;
        return true;
    }
    setTargetTemp(m_config.target_temp);
    m_Time = Selftest.m_Time;
    m_MeasureStartTime = m_Time;
    m_Temp = 0;
    m_TempCount = 0;
    break;
#endif // 0
}

LoopResult CSelftestPart_Heater::stateMeasure() {
    if (int(m_EndTime - SelftestInstance().GetTime()) > 0) {
        // time based progress
        actualizeProgress(SelftestInstance().GetTime(), m_StartTime, m_EndTime);
        return LoopResult::RunCurrent;
    }
#if (0)
    // used to be commented code I just moved it and wrapped in #if (0) instead
    if ((Selftest.m_Time - m_Time) < TEMP_MEASURE_CYCLE_DELAY) {
        float temp = m_config.getTemp();
        m_Temp += temp;
        m_TempCount++;
        return true;
    }
    m_Temp /= m_TempCount;
    if ((m_Time - m_MeasureStartTime) < m_config.heat_time_ms) {
        m_Time = Selftest.m_Time;
        m_Temp = 0;
        m_TempCount = 0;
        return true;
    }
#endif // 0

    // Adapt test to HW differences
    int16_t hw_diff = 0;
    if (m_config.type == heater_type_t::Nozzle) {
        // Bounds check, there might be invalid value in the config_store
        const auto hotend_type = static_cast<size_t>(config_store().hotend_type.get(m_config.tool_nr));
        hw_diff += m_config.hotend_type_temp_offsets[hotend_type < static_cast<size_t>(HotendType::_cnt) ? hotend_type : 0];
    }

    if (hw_diff) {
        log_info(Selftest, "%s heat range offseted by %d degrees Celsius due to HW differences", m_config.partname, hw_diff);
    }

    if ((m_config.getTemp() < m_config.heat_min_temp + hw_diff) || (m_config.getTemp() > m_config.heat_max_temp + hw_diff)) {
        log_error(Selftest, "%s %d out of range (%d - %d)\n", m_config.partname, static_cast<int>(m_config.getTemp()),
            static_cast<int>(m_config.heat_min_temp + hw_diff), static_cast<int>(m_config.heat_max_temp + hw_diff));
        return LoopResult::Fail;
    }
    log_info(Selftest, "%s measure, target: %d current: %f", m_config.partname,
        static_cast<int>(m_config.target_temp), static_cast<double>(m_config.getTemp()));
    return LoopResult::RunNext;
}

#if HAS_SELFTEST_POWER_CHECK()
LoopResult CSelftestPart_Heater::stateCheckLoadChecked() {
    if (!power_check_passed) {
        return LoopResult::Fail;
    }
    return LoopResult::RunNext;
}
#endif

void CSelftestPart_Heater::actualizeProgress(float current, float progres_start, float progres_end) const {
    if (progres_start >= progres_end) {
        return; // don't have estimated end set correctly
    }
    uint8_t current_progress = scale_percent_avoid_overflow(current, progres_start, progres_end);
    rResult.progress = std::max(rResult.progress, current_progress); // heater progress can only rise
}

// Currently supported only by XL, others needs to implement sensor reading, MK4 uses PowerCheckBoth to check its linked heaters
#if HAS_SELFTEST_POWER_CHECK_SINGLE()
void CSelftestPart_Heater::single_check_callback() {
    assert(m_config.type == heater_type_t::Nozzle || m_config.type == heater_type_t::Bed);

    float voltage;
    float current;
    uint32_t pwm;
    float power;

    if (m_config.type == heater_type_t::Nozzle) {
        current = advancedpower.get_nozzle_current(m_config.tool_nr); // This will either give 1.5 A or 0 depending if PWM is on or off
        voltage = advancedpower.get_nozzle_voltage(m_config.tool_nr);
        pwm = advancedpower.get_nozzle_pwm(m_config.tool_nr);
        power = current * voltage;

        /**
         * @note No averaging here.
         * The internal model control in Dwarf is from MINI.
         * It will turn off the output once in a while to follow a curve.
         * @todo Completely retune the PID in dwarf.
         */
    } else {
        voltage = 24; // Modular bed does not measure this
        current = advancedpower.get_bed_current();
        pwm = thermalManager.temp_bed.soft_pwm_amount;
        power = current * voltage;

        // Filter both power and pwm using floating average to filter out sudden changes
        power_avg = (power_avg * 99 + power) / 100;
        pwm_avg = (pwm_avg * 99 + pwm) / 100;
        power = power_avg;
        pwm = pwm_avg;
    }

    LogDebugTimed(
        check_log,
        "%s %fV, %fA, %fW, pwm %d",
        m_config.partname,
        static_cast<double>(voltage),
        static_cast<double>(current),
        static_cast<double>(power),
        static_cast<int>(pwm));

    if (check.EvaluateHeaterStatus(pwm, m_config) == PowerCheck::status_t::stable) {
        PowerCheck::load_t result = check.EvaluateLoad(pwm, power, m_config);
        if (result != PowerCheck::load_t::in_range) {
            state_machine.Fail();
            log_error(Selftest, "%s %s.", m_config.partname, PowerCheck::LoadTexts(result));
        }
        power_check_passed = true;
    }
}
#endif
