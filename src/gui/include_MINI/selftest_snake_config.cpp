#include "selftest_snake_config.hpp"
#include <selftest_types.hpp>
#include <screen_menu_selftest_snake_result_parsing.hpp>

namespace SelftestSnake {
TestResult get_test_result(Action action, Tool tool) {
    SelftestResult sr;
    eeprom_get_selftest_results(&sr);

    switch (action) {
    case Action::Fans:
        return merge_hotends_evaluations(
            [&](int8_t e) {
                return evaluate_results(sr.tools[e].printFan, sr.tools[e].heatBreakFan);
            });
    case Action::XYCheck:
        return evaluate_results(sr.xaxis, sr.yaxis);
    case Action::ZCheck:
        return evaluate_results(sr.zaxis);
    case Action::Heaters:
        return evaluate_results(sr.bed, merge_hotends_evaluations([&](int8_t e) {
            return evaluate_results(sr.tools[e].nozzle);
        }));
    case Action::_count:
        break;
    }
    return TestResult_Unknown;
}

uint8_t get_tool_mask(Tool tool) {
    return ToolMask::AllTools;
}

uint64_t get_test_mask(Action action) {
    switch (action) {
    case Action::Fans:
        return stmFans;
    case Action::XYCheck:
        return stmXYAxis;
    case Action::ZCheck:
        return stmZAxis;
    case Action::Heaters:
        return stmHeaters;
    case Action::_count:
        break;
    }
    assert(false);
    return stmNone;
}

Tool get_last_enabled_tool() {
    return Tool::Tool1;
}
}
