#pragma once

#include <test_result.hpp>
#include <option/has_selftest_dependencies.h>
#include <selftest_action_helpers.hpp>

namespace SelftestSnake {

bool is_completed(TestResult test_result);

#if HAS_SELFTEST_DEPENDENCIES()

using Dependencies = EnumBitset<Action, Action::_count>;

/// @returns dependencies of the action and transitive closure of dependencies of those dependencies
Dependencies all_transitive_dependencies(Action action);

bool are_dependencies_met(Action action);
bool are_all_actions_completed();
void show_unmet_dependencies_warning(Action action);

#else

bool are_previous_completed(Action action);

#endif

}; // namespace SelftestSnake
