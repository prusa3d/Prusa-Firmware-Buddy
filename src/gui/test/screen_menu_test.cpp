/**
 * @file screen_menu_test.cpp
 */

#include "screen_menu_test.hpp"
#include "ScreenHandler.hpp"
#include "WindowMenuLabel.hpp"
#include "sys.h"
#include "window_types.hpp"
#include <cstddef>
#if HAS_SELFTEST()
    #include "test_of_selftest_result.hpp"
    #include "screen_test_selftest.hpp"
#endif

MI_RESULT_TEST::MI_RESULT_TEST()
    : WI_LABEL_t(_("test selftest result"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
}

void MI_RESULT_TEST::click([[maybe_unused]] IWindowMenu &window_menu) {
#if HAS_SELFTEST()
    Screens::Access()->Open(ScreenFactory::Screen<TestResultScreen>);
#endif // HAS_SELFTEST
}

MI_SELFTEST_TEST::MI_SELFTEST_TEST()
    : WI_LABEL_t(_("selftest print screens"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
}

void MI_SELFTEST_TEST::click([[maybe_unused]] IWindowMenu &window_menu) {
#if HAS_SELFTEST()
    Screens::Access()->Open(ScreenFactory::Screen<ScreenTestSelftest>);
#endif // HAS_SELFTEST
}

MI_LOAD_UNLOAD_TEST::MI_LOAD_UNLOAD_TEST()
    : WI_LABEL_t(_("test of load dialog"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
}

void MI_LOAD_UNLOAD_TEST::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenTestMMU>);
}

// TODO rewrite this tests
#if 0
    , tst_graph(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_graph()->id);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"temp graph"))
    , tst_temperature(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_temperature()->id);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"temp - pwm"))
    , tst_heat_err(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*("TEST BED ERROR", "Bed", 1.0, 2.0, 3.0, 4.0);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"HEAT ERROR"))
    , tst_disp_memory(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_disp_mem()->id);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"Disp. R/W"))
#endif // 0

ScreenMenuTest::ScreenMenuTest()
    : ScreenMenuTest__(_(label)) {}
