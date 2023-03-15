/**
 * @file selftest_fans_interface.cpp
 * @author Radek Vana
 * @date 2021-09-24
 */
#include "selftest_fans_interface.hpp" // itself
#include "selftest_fan.h"
#include "selftest_fans_type.hpp"
#include "marlin_server.hpp"
#include "selftest_part.hpp"
#include "eeprom.h"

namespace selftest {

static std::array<SelftestFan_t, HOTENDS> staticResultsPrint;
static std::array<SelftestFan_t, HOTENDS> staticResultsHeatbreak;

// data for both subtests must be sent together
// we could loose some events, so we must be sending entire state of both parts
bool phaseFans(std::array<IPartHandler *, NUM_SELFTEST_FANS> &fans_parts, const std::span<const FanConfig_t> config_fans) {
    if (fans_parts[0] == nullptr) // check only first fan, if null allocate all
    {
        for (size_t i = 0; i < config_fans.size(); i++) {
            auto &result = (config_fans[i].type == fan_type_t::Print) ? staticResultsPrint : staticResultsHeatbreak;

            fans_parts[i] = selftest::Factory::CreateDynamical<CSelftestPart_Fan>(config_fans[i],
                result[config_fans[i].tool_nr],
                &CSelftestPart_Fan::stateStart, &CSelftestPart_Fan::stateWaitStopped,
                &CSelftestPart_Fan::stateCycleMark, &CSelftestPart_Fan::stateWaitRpm,
                &CSelftestPart_Fan::stateMeasureRpm);
        }
    }

    bool any_in_progress = false;
    for (const auto fan_part : fans_parts) {
        if (fan_part == nullptr)
            continue;

        auto res = fan_part->Loop();
        any_in_progress = any_in_progress || res;
    }

    SelftestFans_t result(staticResultsPrint.begin(), staticResultsHeatbreak.begin());
    FSM_CHANGE_WITH_DATA__LOGGING(Selftest, IPartHandler::GetFsmPhase(), result.Serialize());
    if (any_in_progress) {
        return true;
    }

    auto to_TestResult = [](SelftestSubtestState_t subtest) {
        switch (subtest) {
        case SelftestSubtestState_t::not_good:
            return TestResult_Failed;
        case SelftestSubtestState_t::ok:
            return TestResult_Passed;
        default:
            return TestResult_Skipped;
        }
    };

    SelftestResult eeres;
    eeprom_get_selftest_results(&eeres);
    HOTEND_LOOP() {
        eeres.tools[e].printFan = to_TestResult(staticResultsPrint[e].state);
        eeres.tools[e].heatBreakFan = to_TestResult(staticResultsHeatbreak[e].state);
    }
    eeprom_set_selftest_results(&eeres);

    for (size_t i = 0; i < config_fans.size(); i++) {
        delete fans_parts[i];
    }
    std::ranges::fill(fans_parts, nullptr);
    return false;
}
} // namespace selftest
