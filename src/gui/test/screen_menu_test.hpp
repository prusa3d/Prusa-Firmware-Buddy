/**
 * @file screen_menu_test.hpp
 * @brief test screen, accessible in debug only
 */

#pragma once

#include "WindowMenuLabel.hpp"
#include "i_window_menu.hpp"
#include "i_window_menu_item.hpp"
#include "screen_test_gui.hpp"
#include "screen_test_term.hpp"
#include "screen_test_msgbox.hpp"
#include "screen_qr_error.hpp"
#include "screen_test_wizard_icons.hpp"
#include "screen_test_dlg.hpp"
#include "screen_test_load.hpp"

// #include "menu_opener.hpp" TODO make it work
#include "screen_menu.hpp"

class MI_RESULT_TEST : public WI_LABEL_t {
public:
    MI_RESULT_TEST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SELFTEST_TEST : public WI_LABEL_t {
public:
    MI_SELFTEST_TEST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_LOAD_UNLOAD_TEST : public WI_LABEL_t {
public:
    MI_LOAD_UNLOAD_TEST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

using ScreenMenuTest__ = ScreenMenu<EFooter::Off, MI_RETURN,
    MI_LOAD_UNLOAD_TEST,
    /* TODO make it work
    GENERATE_SCREEN_ITEM_DEV(screen_test_gui_data_t, "test GUI"),
    GENERATE_SCREEN_ITEM_DEV(screen_test_term_data_t, "test TERM"),
    GENERATE_SCREEN_ITEM_DEV(ScreenTestMMU, "test of load dialog"),
    GENERATE_SCREEN_ITEM_DEV(ScreenTestMSGBox, "test MSGBOX"),
    GENERATE_SCREEN_ITEM_DEV(screen_test_wizard_icons, "test Wizard icons"),
    GENERATE_SCREEN_ITEM_DEV(screen_test_dlg_data_t, "test dialog"),
    GENERATE_SCREEN_ITEM_DEV(ScreenErrorQR, "test QR error"),*/
    MI_RESULT_TEST, MI_SELFTEST_TEST>;

class ScreenMenuTest : public ScreenMenuTest__ {
public:
    constexpr static const char *label = N_("TEST");
    ScreenMenuTest();
};
