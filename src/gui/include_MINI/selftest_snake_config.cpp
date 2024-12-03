#include "selftest_snake_config.hpp"
#include <selftest_types.hpp>
#include <screen_menu_selftest_snake_result_parsing.hpp>
#include <config_store/store_instance.hpp>
#include <common/SteelSheets.hpp>

namespace SelftestSnake {
TestResult get_test_result(Action action, [[maybe_unused]] Tool tool) {
    SelftestResult sr = config_store().selftest_result.get();

    switch (action) {
    case Action::Fans:
        return merge_hotends_evaluations(
            [&](int8_t e) {
                return evaluate_results(sr.tools[e].evaluate_fans());
            });
    case Action::XYCheck:
        return evaluate_results(sr.xaxis, sr.yaxis);
    case Action::ZCheck:
        return evaluate_results(sr.zaxis);
    case Action::Heaters:
        return evaluate_results(sr.bed, merge_hotends_evaluations([&](int8_t e) {
            return evaluate_results(sr.tools[e].nozzle);
        }));
    case Action::FirstLayer:
        // instead of test result that isn't saved to eeprom, consider calibrated sheet as passed.
        return SteelSheets::IsSheetCalibrated(config_store().active_sheet.get()) ? TestResult_Passed : TestResult_Unknown;
    case Action::_count:
        break;
    }
    return TestResult_Unknown;
}

ToolMask get_tool_mask([[maybe_unused]] Tool tool) {
    return ToolMask::AllTools;
}

uint64_t get_test_mask(Action action) {
    switch (action) {
    case Action::XYCheck:
        return stmXYAxis;
    case Action::ZCheck:
        return stmZAxis;
    case Action::Heaters:
        return stmHeaters;
    case Action::FirstLayer:
        return stmFirstLayer;
    case Action::Fans:
        bsod("get_test_mask");
        break;
    case Action::_count:
        break;
    }
    assert(false);
    return stmNone;
}

} // namespace SelftestSnake
