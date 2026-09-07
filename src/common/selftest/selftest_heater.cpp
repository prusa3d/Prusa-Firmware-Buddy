// selftest_heater.cpp

#include "selftest_heater.h"
#include "hwio.h"
#include "selftest_log.hpp"
#include "fanctl.hpp"
#include "../../Marlin/src/module/temperature.h"
#include "i_selftest.hpp"
#include "algorithm_scale.hpp"
#include <common/marlin_server.hpp>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

#include "advanced_power.hpp"
#include <printers.h>
#include "config_store/store_instance.hpp"
#include <feature/safety_timer/safety_timer.hpp>
#include <bsod/bsod.h>

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
    , last_progress(0)
    , log(2000)
    , check_log(3000) {}

// teardown lives in the destructor because the selftest FSM has no "finally" state
// stateSetup don't touch the heater config (which would bsod on INDX without a picked tool).
CSelftestPart_Heater::~CSelftestPart_Heater() {
    if (!teardown_needed) {
        return;
    }
    log_info(Selftest, "%s finish, target: %d current: %f", m_config.partname,
        static_cast<int>(m_config.target_temp), static_cast<double>(m_config.getTemp().value_or(NAN)));
    m_config.setTargetTemp(0);
    m_config.set_pid(original_pid);
    log_info(Selftest, "%s heater PID regulator restored", m_config.partname);
}

uint32_t CSelftestPart_Heater::estimate(const HeaterConfig_t &config) {
    return config.heat_time_ms;
}

PhysicalToolIndex CSelftestPart_Heater::get_picked_tool() {
#if HAS_INDX()
    const auto maybe_tool = PhysicalToolIndex::currently_selected_opt();
    if (!maybe_tool.has_value()) {
        bsod_unreachable();
    }
    return *maybe_tool;
#else
    return m_config.tool_nr;
#endif
}

LoopResult CSelftestPart_Heater::stateCheckHbrPassed() {
    SelftestResult eeres = config_store().selftest_result.get();
    // Hotfix: skip only on fail so unknown/not-run doesn't abort the test
    // Proper fix: declare fan selftest as a dependency of heater selftest in selftest_snake_config for all printers
    if (eeres.get_heatbreak_fan(m_config.tool_nr) == TestResult::failed) {
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

LoopResult CSelftestPart_Heater::stateShowPickupScreen() {
#if HAS_INDX()
    if (m_config.type == heater_type_t::Nozzle && std::holds_alternative<NoTool>(PhysicalToolIndex::currently_selected())) {
        IPartHandler::SetFsmPhase(PhasesSelftest::Heaters_PickingTool);
        need_pickup = true;
    }
#endif
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateInit() {
#if HAS_INDX()
    if (need_pickup) {
        bool picked = prusa_toolchanger.pick_any_tool(tool_return_t::no_return, {}, tool_change_lift_t::no_lift, false);
        if (!picked) {
            rResult.prep_state = SelftestSubtestState_t::undef;
            rResult.heat_state = SelftestSubtestState_t::undef;
            return LoopResult::Abort;
        }
    }
#elif HAS_TOOLCHANGER()
    // if this tool is not enabled, end this test immediately and set result to undefined
    if (m_config.type == heater_type_t::Nozzle && !m_config.tool_nr.is_enabled()) {
        m_StartTime = m_EndTime = SelftestInstance().GetTime();
        rResult.prep_state = SelftestSubtestState_t::undef;
        rResult.heat_state = SelftestSubtestState_t::undef;
        return LoopResult::Abort;
    }
#endif

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateSetup() {
    if (!teardown_needed) {
        // snapshot before set_pid below - teardown is now necessary
        original_pid = m_config.get_pid();
        teardown_needed = true;
    }
    // INDX has multiple docks but a single hotend, so it behaves like singletool here.
    // XL multitool runs per-tool tests with phases driven by the orchestrator (pretty ugly imo)
#if HAS_TOOLCHANGER() && !HAS_INDX()
    if (prusa_toolchanger.get_num_enabled_tools() <= 1)
#endif
    {
        IPartHandler::SetFsmPhase(PhasesSelftest::Heaters);
    }

    // looked into marlin and it seems all PID values are used as numerator
    // switch regulator into on/off mode
    m_config.set_pid(PID_t {
        .Kp = 1000000,
        .Ki = 0,
        .Kd = 0,
    });

    log_info(Selftest, "%s Started", m_config.partname);
    log_info(Selftest, "%s target: %d current: %f", m_config.partname,
        static_cast<int>(m_config.target_temp), static_cast<double>(m_config.getTemp().value_or(NAN)));
    log_info(Selftest, "%s heater PID regulator changed to P regulator", m_config.partname);

    m_StartTime = SelftestInstance().GetTime();
    m_EndTime = m_StartTime + estimate(m_config);
    // m_TempDiffSum = 0;
    // m_TempDiffSum = 0;
    // m_TempCount = 0;
    const auto temp_opt = m_config.getTemp();
    if (!temp_opt.has_value()) {
        // No valid reading yet, wait until we get a valid temperature
        return LoopResult::RunCurrent;
    }
    const float temp = temp_opt.value();
    begin_temp = temp;
    enable_cooldown = temp >= m_config.start_temp;
    m_config.setTargetTemp(0);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateTakeControlOverFans() {
    log_info(Selftest, "%s took control of fans", m_config.partname);
    const auto tool = get_picked_tool();
    m_config.print_fan_fnc(tool).enter_selftest_mode();
    m_config.heatbreak_fan_fnc(tool).enter_selftest_mode();
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateFansActivate() {
    const auto tool = get_picked_tool();
    if (enable_cooldown) {
        log_info(Selftest, "%s set fans to maximum", m_config.partname);
        m_config.print_fan_fnc(tool).selftest_set_pwm(255); // it will be restored by exitSelftestMode
        m_config.heatbreak_fan_fnc(tool).selftest_set_pwm(255); // it will be restored by exitSelftestMode
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
    const auto temp_opt = m_config.getTemp();
    if (!temp_opt.has_value()) {
        // No reading right now — wait for the next manage tick.
        return LoopResult::RunCurrent;
    }
    const float temp = temp_opt.value();

    if (!enable_cooldown) {
        log_info(Selftest, "%s cooldown not needed, target: %d current: %f", m_config.partname,
            static_cast<int>(m_config.target_temp), static_cast<double>(temp));
        return LoopResult::RunNext;
    }

    LogInfoTimed(log, "%s cooling down, target: %d current: %f", m_config.partname,
        static_cast<int>(m_config.target_temp), static_cast<double>(temp));

    if (temp > m_config.undercool_temp) {
        // m_config.undercool_temp .. 100%
        // begin_temp              ..   0%
        actualizeProgress(temp, begin_temp, m_config.undercool_temp);
        return LoopResult::RunCurrent;
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateFansDeactivate() {
    const auto tool = get_picked_tool();
    m_config.print_fan_fnc(tool).exit_selftest_mode();
    m_config.heatbreak_fan_fnc(tool).exit_selftest_mode();
    log_info(Selftest, "%s returned control of fans", m_config.partname);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateTargetTemp() {
    log_info(Selftest, "%s set target, target: %d current: %f", m_config.partname,
        static_cast<int>(m_config.target_temp), static_cast<double>(m_config.getTemp().value_or(NAN)));
    rResult.prep_state = SelftestSubtestState_t::running; // waiting for preheat temperature
    m_config.setTargetTemp(m_config.target_temp);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateWait() {
    const auto temp_opt = m_config.getTemp();
    if (!temp_opt.has_value()) {
        // No reading right now — wait for the next manage tick.
        return LoopResult::RunCurrent;
    }
    const float current_temp = temp_opt.value();
    if (current_temp >= m_config.start_temp) {
        rResult.prep_state = SelftestSubtestState_t::ok; // preheat temperature ok
        rResult.heat_state = SelftestSubtestState_t::running; // waiting final heat
        m_MeasureStartTime = SelftestInstance().GetTime();
        m_StartTime = SelftestInstance().GetTime();
        m_EndTime = m_StartTime + estimate(m_config);
        rResult.progress = 0;
        log_info(Selftest, "%s wait start temp reached: target: %d current: %f", m_config.partname,
            static_cast<int>(m_config.target_temp), static_cast<double>(current_temp));
        return LoopResult::RunNext;
    }
    LogInfoTimed(log, "%s wait, run: target: %d current: %f", m_config.partname,
        static_cast<int>(m_config.target_temp), static_cast<double>(current_temp));

    // m_config.start_temp     .. 100%
    // m_config.undercool_temp ..   0%
    actualizeProgress(current_temp, m_config.undercool_temp, m_config.start_temp);
    return LoopResult::RunCurrent;

#if (0)
    // #error dead code found by automatic analyses (see BFW-5461)
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
    // #error dead code found by automatic analyses (see BFW-5461)
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

#if HAS_HOTEND_TYPE_SUPPORT()
    if (m_config.type == heater_type_t::Nozzle) {
        hw_diff += hotend_type_heater_selftest_offset(config_store().hotend_type.get(m_config.tool_nr.to_raw()));
    }
#endif

    if (hw_diff) {
        log_info(Selftest, "%s heat range offseted by %d degrees Celsius due to HW differences", m_config.partname, hw_diff);
    }

    // we are measuring how long it takes to heat up to temp in (heat_min_temp, heat_max_temp) interval
    // target_temp must be big enough to keep PID at full power
    debug_assert(m_config.target_temp > m_config.heat_max_temp + 10);

    const auto temp_opt = m_config.getTemp();
    if (!temp_opt.has_value()) {
        // Can't evaluate the range without a reading — wait for the next manage tick.
        // Persistent nullopt means the tool is no longer selected; user must Abort.
        return LoopResult::RunCurrent;
    }
    const float temp = temp_opt.value();
    if ((temp < m_config.heat_min_temp + hw_diff) || (temp > m_config.heat_max_temp + hw_diff)) {
        log_error(Selftest, "%s %d out of range (%d - %d)\n", m_config.partname, static_cast<int>(temp),
            static_cast<int>(m_config.heat_min_temp + hw_diff), static_cast<int>(m_config.heat_max_temp + hw_diff));
        return LoopResult::Fail;
    }
    log_info(Selftest, "%s measure, target: %d current: %f", m_config.partname,
        static_cast<int>(m_config.target_temp), static_cast<double>(temp));
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
    uint8_t current_progress = static_cast<uint8_t>(scale_percent_avoid_overflow(current, progres_start, progres_end));
    rResult.progress = std::max(rResult.progress, current_progress); // heater progress can only rise
}

// Currently supported only by XL, others needs to implement sensor reading, MK4 uses PowerCheckBoth to check its linked heaters
#if HAS_SELFTEST_POWER_CHECK_SINGLE()
void CSelftestPart_Heater::single_check_callback() {
    debug_assert(m_config.type == heater_type_t::Nozzle || m_config.type == heater_type_t::Bed);

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
        pwm = static_cast<uint32_t>(pwm_avg);
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
