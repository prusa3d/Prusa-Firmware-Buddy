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
#include <config_store/store_instance.hpp>
#include <option/has_switched_fan_test.h>

namespace selftest {

static std::array<SelftestFanHotendResult, HOTENDS> static_hotend_results;

// data for both subtests must be sent together
// we could loose some events, so we must be sending entire state of both parts
bool phaseFans(std::array<IPartHandler *, HOTENDS> &fans_parts, const std::span<const SelftestFansConfig> fans_configs) {
    if (fans_parts[0] == nullptr) // check only first fan, if null allocate all
    {
        for (size_t i = 0; i < fans_parts.size(); i++) {
            fans_parts[i] = selftest::Factory::CreateDynamical<CSelftestPart_Fan>(fans_configs[i],
                static_hotend_results[i],
                &CSelftestPart_Fan::state_start,
                &CSelftestPart_Fan::state_wait_rpm_100_percent,
                &CSelftestPart_Fan::state_measure_rpm_100_percent,
#if PRINTER_IS_PRUSA_MK3_5() && HAS_SWITCHED_FAN_TEST()
                // Adding only for MK3.5 because selftest is unable to skip states. This seems to me as a cleaner choice than checking in every state for basically the same thing.
                &CSelftestPart_Fan::state_manual_check_init,
                &CSelftestPart_Fan::state_manual_check_wait_fan,
                &CSelftestPart_Fan::state_manual_check_ask,
#endif
                &CSelftestPart_Fan::state_rpm_0_init,
                &CSelftestPart_Fan::state_wait_rpm_0_percent,
                &CSelftestPart_Fan::state_wait_rpm_40_percent,
                &CSelftestPart_Fan::state_measure_rpm_40_percent);
        }
    }

    bool any_in_progress = false;
    for (const auto fan_part : fans_parts) {
        if (fan_part == nullptr) {
            continue;
        }

        auto res = fan_part->Loop();
        any_in_progress = any_in_progress || res;
    }

    SelftestFansResult result(static_hotend_results);
    marlin_server::fsm_change_extended(IPartHandler::GetFsmPhase(), result);
    if (any_in_progress) {
        return true;
    }

    auto to_test_result = [](SelftestSubtestState_t state) {
        switch (state) {
        case SelftestSubtestState_t::not_good:
            return TestResult_Failed;
        case SelftestSubtestState_t::ok:
            return TestResult_Passed;
        default:
            return TestResult_Skipped;
        }
    };

    SelftestResult eeres = config_store().selftest_result.get();
    for (size_t i = 0; i < static_hotend_results.size(); ++i) {
        const auto &hr = static_hotend_results[i];

        eeres.tools[i].printFan = to_test_result(hr.print_fan_state);
        eeres.tools[i].heatBreakFan = to_test_result(hr.heatbreak_fan_state);
#if HAS_SWITCHED_FAN_TEST()
        eeres.tools[i].fansSwitched = to_test_result(hr.fans_switched_state);
#endif /* HAS_SWITCHED_FAN_TEST() */
    }

    config_store().selftest_result.set(eeres);

    for (size_t i = 0; i < fans_parts.size(); i++) {
        delete fans_parts[i];
    }
    std::ranges::fill(fans_parts, nullptr);

    return false;
}

} // namespace selftest
