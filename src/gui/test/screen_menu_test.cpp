/**
 * @file screen_menu_test.cpp
 */

#include "screen_menu_test.hpp"
#include "ScreenHandler.hpp"
#include "i_window_menu_item.hpp"
#include "sys.h"
#include "window_types.hpp"
#include <cstddef>
#if HAS_SELFTEST()
    #include "test_of_selftest_result.hpp"
#endif

MI_RESULT_TEST::MI_RESULT_TEST()
    : IWindowMenuItem(_("test selftest result"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
}

void MI_RESULT_TEST::click([[maybe_unused]] IWindowMenu &window_menu) {
#if HAS_SELFTEST()
    Screens::Access()->Open(ScreenFactory::Screen<TestResultScreen>);
#endif // HAS_SELFTEST
}

ScreenMenuTest::ScreenMenuTest()
    : ScreenMenuTest__(_(label)) {}
