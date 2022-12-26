#include "MItem_basic_selftest.hpp"
#include "marlin_client.h"
#include "gui.hpp"
#include "sys.h"
#include "DialogHandler.hpp"
#include "ScreenHandler.hpp"
#include "ScreenFactory.hpp"
#include "printer_selftest.hpp"
#include "main.h"
#include "Pin.hpp"
#include "hwio_pindef.h"
#include "config.h"
#include "menu_spin_config.hpp"
#include "ScreenSelftest.hpp"

/*****************************************************************************/
//MI_WIZARD
MI_WIZARD::MI_WIZARD()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_WIZARD::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmWizard);
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
}

/*****************************************************************************/
//MI_SELFTEST
MI_SELFTEST::MI_SELFTEST()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SELFTEST::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmFullSelftest);
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
}

/*****************************************************************************/
//MI_SELFTEST_RESULT
MI_SELFTEST_RESULT::MI_SELFTEST_RESULT()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SELFTEST_RESULT::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmShow_result);
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
}

/*****************************************************************************/
//MI_CALIB_FIRST
MI_CALIB_FIRST::MI_CALIB_FIRST()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, PRINTER_TYPE == PRINTER_PRUSA_MINI ? is_hidden_t::no : is_hidden_t::dev) {
}

void MI_CALIB_FIRST::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmFirstLayer);
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
}

/*****************************************************************************/
//MI_TEST_FANS
MI_TEST_FANS::MI_TEST_FANS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEST_FANS::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmFans);
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
}

/*****************************************************************************/
//MI_TEST_XYZ
MI_TEST_XYZ::MI_TEST_XYZ()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEST_XYZ::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmXYZAxis);
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
}

/*****************************************************************************/
//MI_TEST_X
MI_TEST_X::MI_TEST_X()
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_TEST_X::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmXAxis);
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
}

/*****************************************************************************/
//MI_TEST_Y
MI_TEST_Y::MI_TEST_Y()
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_TEST_Y::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmYAxis);
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
}

/*****************************************************************************/
//MI_TEST_Z
MI_TEST_Z::MI_TEST_Z()
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_TEST_Z::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmZAxis);
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
}

/*****************************************************************************/
//MI_TEST_HEAT
MI_TEST_HEAT::MI_TEST_HEAT()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEST_HEAT::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmHeaters);
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
}

/*****************************************************************************/
//MI_TEST_HOTEND
MI_TEST_HOTEND::MI_TEST_HOTEND()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_TEST_HOTEND::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmHeaters_noz);
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
}

/*****************************************************************************/
//MI_TEST_BED
MI_TEST_BED::MI_TEST_BED()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_TEST_BED::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmHeaters_bed);
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
}

/*****************************************************************************/
//MI_TEST_FANS_fine
MI_ADVANCED_FAN_TEST::MI_ADVANCED_FAN_TEST()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_ADVANCED_FAN_TEST::click(IWindowMenu & /*window_menu*/) {
    marlin_test_start(stmFans_fine);
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
}
