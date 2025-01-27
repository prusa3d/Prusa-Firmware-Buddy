#pragma once
#include <common/fsm_base_types.hpp>
#include "option/filament_sensor.h"
#include "selftest_result.hpp"

constexpr const char *ToString(TestResult res) {
    switch (res) {
    case TestResult_Unknown:
        return "Unknown";
    case TestResult_Skipped:
        return "Skipped";
    case TestResult_Passed:
        return "Passed";
    case TestResult_Failed:
        return "Failed";
    default:
        break;
    }
    return "ERROR";
}

constexpr const char *ToString(TestResultNet res) {
    switch (res) {
    case TestResultNet_Unknown:
        return "Unknown";
    case TestResultNet_Unlinked:
        return "Unlinked";
    case TestResultNet_Down:
        return "Down";
    case TestResultNet_NoAddress:
        return "NoAddress";
    case TestResultNet_Up:
        return "Up";
    default:
        break;
    }
    return "ERROR";
}

/**
 * @brief Know globally if selftest passed.
 * Currently not affected by eth and wifi.
 */
bool SelftestResult_Passed_All(const SelftestResult &results);

/**
 * @brief Checks mandatory tests and if optional didn't fail
 * Currently only for MK4, for others it works the same as All
 */
bool SelftestResult_Passed_Mandatory(const SelftestResult &results);

/**
 * @brief Know globally if selftest failed.
 * Currently not affected by eth and wifi.
 */
bool SelftestResult_Failed(const SelftestResult &results);

/**
 * @brief Log all results.
 */
void SelftestResult_Log(const SelftestResult &results);

/**
 * @brief FSM compatible structure to request testing selftest results screen.
 */
class FsmSelftestResult {
    fsm::PhaseData data = {};

public:
    constexpr FsmSelftestResult(uint8_t test_selftest_code) {
        data[0] = 0xff; // Show fake results to test GUI screen
        data[1] = test_selftest_code;
    }

    constexpr FsmSelftestResult() {
        data[0] = 0; // Show results from EEPROM
    }

    constexpr FsmSelftestResult(fsm::PhaseData new_data) {
        Deserialize(new_data);
    }

    constexpr bool is_test_selftest() const {
        return (data[0] != 0);
    }

    constexpr uint8_t test_selftest_code() const {
        return data[1];
    }

    constexpr fsm::PhaseData Serialize() const {
        return data;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        data = new_data;
    }
};
