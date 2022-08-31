/**
 * @file screen_test.cpp
 * @brief test screen, accessible in debug only
 */

#include "config.h"
#include "stm32f4xx_hal.h"
#include "bsod.h"
#include "ScreenHandler.hpp"
#include "screen_test_gui.hpp"
#include "screen_test_term.hpp"
#include "screen_test_msgbox.hpp"
#include "screen_test_wizard_icons.hpp"
#include "screen_test_dlg.hpp"
#include "screen_menu_eeprom_test.hpp"
#include "screen_test_selftest.hpp"
#include "WindowMenuItems.hpp"
#include "screen_menu.hpp"

//generate stack overflow
static volatile int _recursive = 1;
static volatile void recursive(uint64_t i) {
    uint64_t x = i + (uint64_t)_recursive;
    osDelay(1);
    if (_recursive)
        recursive(x);
}

class TstEeprom : public WI_LABEL_t {
    static constexpr const char *const label = "EEPROM"; // intentionally not translated

public:
    TstEeprom()
        : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((const uint8_t *)label)) {
    }

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open(GetScreenMenuEepromTest);
    }
};

class TstGui : public WI_LABEL_t {
    static constexpr const char *const label = "GUI"; // intentionally not translated

public:
    TstGui()
        : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((const uint8_t *)label)) {
    }

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open(ScreenFactory::Screen<screen_test_gui_data_t>);
    }
};

class TstSelftest : public WI_LABEL_t {
    static constexpr const char *const label = "SELFTEST"; // intentionally not translated

public:
    TstSelftest()
        : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((const uint8_t *)label)) {
    }

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open(ScreenFactory::Screen<ScreenTestSelftest>);
    }
};

class TstTerm : public WI_LABEL_t {
    static constexpr const char *const label = "Terminal"; // intentionally not translated

public:
    TstTerm()
        : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((const uint8_t *)label)) {
    }

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open(ScreenFactory::Screen<screen_test_term_data_t>);
    }
};

class TstMsgBox : public WI_LABEL_t {
    static constexpr const char *const label = "Message box"; // intentionally not translated

public:
    TstMsgBox()
        : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((const uint8_t *)label)) {
    }

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open(ScreenFactory::Screen<screen_test_msgbox_data_t>);
    }
};

class TstWizardIcons : public WI_LABEL_t {
    static constexpr const char *const label = "Wizard Icons"; // intentionally not translated

public:
    TstWizardIcons()
        : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((const uint8_t *)label)) {
    }

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open(ScreenFactory::Screen<screen_test_wizard_icons>);
    }
};

class TstSafetyDialog : public WI_LABEL_t {
    static constexpr const char *const label = "Dialog"; // intentionally not translated

public:
    TstSafetyDialog()
        : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((const uint8_t *)label)) {
    }

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Open(ScreenFactory::Screen<screen_test_dlg_data_t>);
    }
};

class TstStackOverflow : public WI_LABEL_t {
    static constexpr const char *const label = "Stack overflow"; // intentionally not translated

public:
    TstStackOverflow()
        : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((const uint8_t *)label)) {
    }

protected:
    virtual void click(IWindowMenu &window_menu) override {
        recursive(0);
    }
};

class TstDiv0 : public WI_LABEL_t {
    static constexpr const char *const label = "BSOD div 0"; // intentionally not translated

public:
    TstDiv0()
        : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((const uint8_t *)label)) {
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

using Screen = ScreenMenu<EFooter::Off, MI_RETURN, TstEeprom, TstGui, TstSelftest, TstTerm, TstMsgBox, TstWizardIcons, TstSafetyDialog, TstStackOverflow, TstDiv0>;

class ScreenMenuTest : public Screen {
public:
    constexpr static const char *label = "TEST";
    ScreenMenuTest()
        : Screen(string_view_utf8::MakeCPUFLASH((const uint8_t *)label)) {
    }
};

ScreenFactory::UniquePtr GetScreenMenuTest() {
    return ScreenFactory::Screen<ScreenMenuTest>();
}

// Missing tests we used to have (currently incompatible code)
#if 0
    , tst_graph(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_graph()->id);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"temp graph"))
    , tst_temperature(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_temperature()->id);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"temp - pwm"))
    , tst_heat_err(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*("TEST BED ERROR", "Bed", 1.0, 2.0, 3.0, 4.0);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"HEAT ERROR"))
    , tst_disp_memory(this, this->GenerateRect(ShiftDir_t::Bottom), []() { /*screen_open(get_scr_test_disp_mem()->id);*/ }, string_view_utf8::MakeCPUFLASH((const uint8_t *)"Disp. R/W"))
#endif // 0
