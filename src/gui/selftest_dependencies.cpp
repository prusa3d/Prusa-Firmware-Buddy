#include "selftest_dependencies.hpp"

#include <string_builder.hpp>
#include <window_msgbox.hpp>
#include <utils/bitset_utils.hpp>
#include <printers.h>
#include <option/has_nextruder.h>

namespace SelftestSnake {

bool is_completed(TestResult test_result) {
    // Skipped is also considered completed - it marks non-obligatory tests that have been explicitly skipped by the user
    return test_result == TestResult::passed || test_result == TestResult::skipped;
}

#if HAS_SELFTEST_DEPENDENCIES()

Dependencies all_transitive_dependencies(Action action) {
    return bitset_flood_fill(get_dependencies(action), [](size_t i) { return get_dependencies(static_cast<Action>(i)); });
}

bool are_dependencies_met(Action action) {
    const auto dependencies = get_dependencies(action);
    for (Action dependency : valid_actions()) {
        if (!dependencies.test(dependency)) {
            continue;
        }
        if (!is_completed(get_test_result(dependency, AllTools {}))) {
            return false;
        }
    }
    return true;
}

bool are_all_actions_completed() {
    for (Action action : valid_actions()) {
        if (!is_completed(get_test_result(action, AllTools {}))) {
            return false;
        }
    }
    return true;
}

void show_unmet_dependencies_warning(Action action) {
    // Assume 3 bytes per symbol at most (Katakana)
    constexpr int msg_size = 3 * (sizeof("Complete these calibrations first:") + 4 * sizeof("Filament Sensor Calibration"));
    ArrayStringBuilder<msg_size> sb;
    sb.append_string_view(_("Complete these calibrations first:"));
    const auto dependencies = all_transitive_dependencies(action);
    for (Action dependency : valid_actions()) {
        if (!dependencies.test(dependency)) {
            continue;
        }
        if (!is_completed(get_test_result(dependency, AllTools {}))) {
            sb.append_printf("\n- ");
            sb.append_string_view(_(get_action_label(dependency)));
        }
    }

    // The string may overflow if there is too many dependencies
    MsgBoxWarning(string_view_utf8::MakeRAM(sb.str_nocheck()), Responses_Ok);
}

consteval bool check_selftest_ordering() {
    for (auto i = 0; i < static_cast<int>(Action::_count); i++) {
        const auto deps = get_dependencies(static_cast<Action>(i));
        for (auto j = i; j < static_cast<int>(Action::_count); j++) {
            // selftest j goes after i -> if j has dependency on i the ordering is wrong
            if (deps.test(static_cast<Action>(j))) {
                return false;
            }
        }
    }
    return true;
}

static_assert(check_selftest_ordering(), "selftests ordering does not satisfy dependencies");

#else

bool are_previous_completed(Action action) {
    for (Action act : valid_actions()) {
        if (act == action) {
            break;
        }
        if (!is_completed(get_test_result(act, AllTools {}))) {
            return false;
        }
    }

    return true;
}

#endif

#if HAS_NEXTRUDER() && !PRINTER_IS_PRUSA_iX()
static_assert(Action::Gears < Action::FilamentSensorCalibration, "Filament could get locked up in the gearbox");
#endif

}; // namespace SelftestSnake
