/**
 * @file screen_test.cpp
 * @brief test screen, accessible in debug only
 */

#include "ScreenHandler.hpp"
#include "screen_test_gui.hpp"
#include "screen_test_term.hpp"
#include "screen_test_msgbox.hpp"
#include "screen_qr_error.hpp"
#include "screen_test_wizard_icons.hpp"
#include "screen_test_dlg.hpp"
#include "screen_menu_eeprom_test.hpp"
#include "feature/selftest.h"
#if HAS_SELFTEST
    #include "test_of_selftest_result.hpp"
    #include "screen_test_selftest.hpp"
#endif
#include "screen_test_load.hpp"

#include "menu_opener.hpp"
#include "screen_menu.hpp"
#include "screen_menus.hpp"

//generate stack overflow
static volatile int _recursive = 1;
static volatile void recursive(uint64_t i) {
    uint64_t x = i + (uint64_t)_recursive;
    osDelay(1);
    if (_recursive)
        recursive(x);
}

class MI_STACK_OVERFLOW : public WI_LABEL_t {
public:
    MI_STACK_OVERFLOW()
        : WI_LABEL_t(_("Stack overflow"), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
    }

protected:
    virtual void click(IWindowMenu &window_menu) override {
        recursive(0);
    }
};

class MI_DIV0 : public WI_LABEL_t {
public:
    MI_DIV0()
        : WI_LABEL_t(_("BSOD div 0"), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
    }

protected:
    virtual void click(IWindowMenu &window_menu) override {
        static volatile int i = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiv-by-zero"
        i = i / 0;
#pragma GCC diagnostic pop
    }
};

//TODO rewrite this tests
#if 0
    , tst_graph(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_graph()->id);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"temp graph"))
    , tst_temperature(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_temperature()->id);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"temp - pwm"))
    , tst_heat_err(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*("TEST BED ERROR", "Bed", 1.0, 2.0, 3.0, 4.0);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"HEAT ERROR"))
    , tst_disp_memory(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_disp_mem()->id);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"Disp. R/W"))
#endif // 0

using Screen = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,

    GENERATE_SCREEN_FN_ITEM_DEV(GetScreenMenuEepromTest, "test EEPROM"),
    GENERATE_SCREEN_ITEM_DEV(screen_test_gui_data_t, "test GUI"),
    GENERATE_SCREEN_ITEM_DEV(screen_test_term_data_t, "test TERM"),
    GENERATE_SCREEN_ITEM_DEV(ScreenTestMMU, "test of load dialog"),
    GENERATE_SCREEN_ITEM_DEV(ScreenTestMSGBox, "test MSGBOX"),
    GENERATE_SCREEN_ITEM_DEV(screen_test_wizard_icons, "test Wizard icons"),
    GENERATE_SCREEN_ITEM_DEV(screen_test_dlg_data_t, "test dialog"),
    GENERATE_SCREEN_ITEM_DEV(ScreenErrorQR, "test QR error"),
#if HAS_SELFTEST
    GENERATE_SCREEN_ITEM_DEV(TestResult, "test selftest result"),
    GENERATE_SCREEN_ITEM_DEV(ScreenTestSelftest, "selftest print screens"),
#endif // HAS_SELFTEST
    MI_STACK_OVERFLOW, MI_DIV0>;

class ScreenMenuTest : public Screen {
public:
    constexpr static const char *label = N_("TEST");
    ScreenMenuTest()
        : Screen(_(label)) {}
};

ScreenFactory::UniquePtr GetScreenMenuTest() {
    return ScreenFactory::Screen<ScreenMenuTest>();
}
