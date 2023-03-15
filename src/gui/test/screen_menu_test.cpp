/**
 * @file screen_menu_test.cpp
 */

#include "screen_menu_test.hpp"
#include "ScreenHandler.hpp"
#if HAS_SELFTEST
    #include "test_of_selftest_result.hpp"
    #include "screen_test_selftest.hpp"
#endif

//generate stack overflow
static volatile int _recursive = 1;
static volatile void recursive(uint64_t i) {
    uint64_t x = i + (uint64_t)_recursive;
    osDelay(1);
    if (_recursive)
        recursive(x);
}

MI_STACK_OVERFLOW::MI_STACK_OVERFLOW()
    : WI_LABEL_t(_("Stack overflow"), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_STACK_OVERFLOW::click(IWindowMenu &window_menu) {
    recursive(0);
}

MI_DIV0::MI_DIV0()
    : WI_LABEL_t(_("BSOD div 0"), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_DIV0::click(IWindowMenu &window_menu) {
    static volatile int i = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiv-by-zero"
    i = i / 0;
#pragma GCC diagnostic pop
}

MI_RESULT_TEST::MI_RESULT_TEST()
    : WI_LABEL_t(_("test selftest result"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
}

void MI_RESULT_TEST::click(IWindowMenu &window_menu) {
#if HAS_SELFTEST
    Screens::Access()->Open(ScreenFactory::Screen<TestResultScreen>);
#endif // HAS_SELFTEST
}

MI_SELFTEST_TEST::MI_SELFTEST_TEST()
    : WI_LABEL_t(_("selftest print screens"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
}

void MI_SELFTEST_TEST::click(IWindowMenu &window_menu) {
#if HAS_SELFTEST
    Screens::Access()->Open(ScreenFactory::Screen<ScreenTestSelftest>);
#endif // HAS_SELFTEST
}

MI_LOAD_UNLOAD_TEST::MI_LOAD_UNLOAD_TEST()
    : WI_LABEL_t(_("test of load dialog"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
}

void MI_LOAD_UNLOAD_TEST::click(IWindowMenu &window_menu) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenTestMMU>);
}

//TODO rewrite this tests
#if 0
    , tst_graph(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_graph()->id);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"temp graph"))
    , tst_temperature(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_temperature()->id);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"temp - pwm"))
    , tst_heat_err(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*("TEST BED ERROR", "Bed", 1.0, 2.0, 3.0, 4.0);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"HEAT ERROR"))
    , tst_disp_memory(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_disp_mem()->id);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"Disp. R/W"))
#endif // 0

ScreenMenuTest::ScreenMenuTest()
    : ScreenMenuTest__(_(label)) {}
