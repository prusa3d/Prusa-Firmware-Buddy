// selftest_heater.h
#pragma once

#include <inttypes.h>
#include "selftest_heaters_type.hpp"
#include "i_selftest_part.hpp"
#include "selftest_heater_config.hpp"
#include "selftest_log.hpp"
#include "power_check.hpp"

namespace selftest {

class CSelftestPart_Heater {
    friend class PowerCheckBoth;

public:
    IPartHandler &state_machine;
    const HeaterConfig_t &m_config;

private:
    SelftestHeater_t &rResult;
    float storedKp;
    float storedKi;
    float storedKd;
    uint32_t m_StartTime;
    uint32_t m_EndTime;
    uint32_t m_MeasureStartTime;
    float begin_temp;
    float last_progress; // cannot go backwards
    // float m_TempDiffSum;
    // float m_TempDeltaSum;
    // uint16_t m_TempCount;
    bool enable_cooldown;
    LogTimer log;

    // a flag indicating that the nozzle heater test should be skipped due to
    // the hotend fan test not having passed
    bool nozzle_test_skipped = false;

#if HAS_SELFTEST_POWER_CHECK()
    // Power check related stuff
    PowerCheck check;
    bool power_check_passed = false;
#endif

    LogTimer check_log;
    float power_avg = 0;
    float pwm_avg = 0;

    static uint32_t estimate(const HeaterConfig_t &config);
    void actualizeProgress(float current, float progres_start, float progres_end) const;

public:
    CSelftestPart_Heater(IPartHandler &state_machine, const HeaterConfig_t &config,
        SelftestHeater_t &result);
    ~CSelftestPart_Heater();

    /// Checks that hotend fan test passed for the tool that is being tested.
    /// Changes FSM state if it hasn't, which shows a dialog on non-XL printers
    /// and a notice on XL (on which only the failed tools are skipped and the
    /// rest is tested).
    LoopResult stateCheckHbrPassed();
    /// Handles the button response on non-XL printers or aborts outright on
    /// XL. If the hotend fan test passed, just continues to next state.
    LoopResult stateShowSkippedDialog();
    /// Sets up the heater test.
    LoopResult stateSetup();
    LoopResult stateTakeControlOverFans(); // also enters fan selftest mode
    LoopResult stateFansActivate();
    LoopResult stateCooldownInit();
    LoopResult stateCooldown();
    LoopResult stateFansDeactivate(); // also exits fan selftest mode
    LoopResult stateTargetTemp();
    LoopResult stateWait();
    LoopResult stateMeasure();

#if HAS_SELFTEST_POWER_CHECK()
    LoopResult stateCheckLoadChecked();
#endif

#if HAS_SELFTEST_POWER_CHECK_SINGLE()
    // Simple check callback for independent heaters
    void single_check_callback();
#endif
};

}; // namespace selftest
