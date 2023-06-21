#include "MItem_basic_selftest.hpp"

/*****************************************************************************/
// MI_WIZARD
MI_WIZARD::MI_WIZARD()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_WIZARD::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_SELFTEST
MI_SELFTEST::MI_SELFTEST()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_SELFTEST::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_SELFTEST_RESULT
MI_SELFTEST_RESULT::MI_SELFTEST_RESULT()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_SELFTEST_RESULT::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_CALIB_FIRST
MI_CALIB_FIRST::MI_CALIB_FIRST()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_CALIB_FIRST::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_TEST_FANS
MI_TEST_FANS::MI_TEST_FANS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_FANS::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_TEST_XYZ
MI_TEST_XYZ::MI_TEST_XYZ()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_XYZ::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_TEST_X
MI_TEST_X::MI_TEST_X()
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_X::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_TEST_Y
MI_TEST_Y::MI_TEST_Y()
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_Y::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_TEST_Z
MI_TEST_Z::MI_TEST_Z()
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_Z::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_TEST_HEAT
MI_TEST_HEAT::MI_TEST_HEAT()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_HEAT::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_TEST_HOTEND
MI_TEST_HOTEND::MI_TEST_HOTEND()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_HOTEND::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_TEST_BED
MI_TEST_BED::MI_TEST_BED()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_BED::click(IWindowMenu & /*window_menu*/) {
}
