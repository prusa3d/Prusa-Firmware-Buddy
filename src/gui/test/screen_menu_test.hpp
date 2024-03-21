/**
 * @file screen_menu_test.hpp
 * @brief test screen, accessible in debug only
 */

#pragma once

#include "i_window_menu_item.hpp"
#include "i_window_menu.hpp"
#include "i_window_menu_item.hpp"
#include "screen_test_gui.hpp"
#include "screen_test_term.hpp"
#include "screen_test_msgbox.hpp"
#include "screen_qr_error.hpp"
#include "screen_test_wizard_icons.hpp"

// #include "menu_opener.hpp" TODO make it work
#include "screen_menu.hpp"

class MI_RESULT_TEST : public IWindowMenuItem {
public:
    MI_RESULT_TEST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

using ScreenMenuTest__ = ScreenMenu<EFooter::Off, MI_RETURN,
    MI_RESULT_TEST>;

class ScreenMenuTest : public ScreenMenuTest__ {
public:
    constexpr static const char *label = N_("TEST");
    ScreenMenuTest();
};
