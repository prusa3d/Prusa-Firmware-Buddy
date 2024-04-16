#include "MItem_basic_selftest.hpp"

/*****************************************************************************/
// MI_WIZARD
MI_WIZARD::MI_WIZARD()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_WIZARD::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_SELFTEST
MI_SELFTEST::MI_SELFTEST()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_SELFTEST::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_SELFTEST_RESULT
MI_SELFTEST_RESULT::MI_SELFTEST_RESULT()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_SELFTEST_RESULT::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_CALIB_FIRST
MI_CALIB_FIRST::MI_CALIB_FIRST()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_CALIB_FIRST::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_TEST_FANS
MI_TEST_FANS::MI_TEST_FANS()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_FANS::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_TEST_XYZ
MI_TEST_XYZ::MI_TEST_XYZ()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_XYZ::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_TEST_X
MI_TEST_X::MI_TEST_X()
    : IWindowMenuItem(string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_X::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_TEST_Y
MI_TEST_Y::MI_TEST_Y()
    : IWindowMenuItem(string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_Y::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_TEST_Z
MI_TEST_Z::MI_TEST_Z()
    : IWindowMenuItem(string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_Z::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_TEST_HEAT
MI_TEST_HEAT::MI_TEST_HEAT()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_HEAT::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_TEST_HOTEND
MI_TEST_HOTEND::MI_TEST_HOTEND()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_HOTEND::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_TEST_BED
MI_TEST_BED::MI_TEST_BED()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_BED::click(IWindowMenu & /*window_menu*/) {
}
