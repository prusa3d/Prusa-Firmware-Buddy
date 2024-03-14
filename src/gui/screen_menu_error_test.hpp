#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include "config_features.h"
#include "screen_menu.hpp"

class MI_TRIGGER_BSOD : public WI_LABEL_t {
public:
    MI_TRIGGER_BSOD();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_STACK_OVERFLOW : public WI_LABEL_t {
public:
    MI_STACK_OVERFLOW();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_DIV0 : public WI_LABEL_t {
public:
    MI_DIV0();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_WATCHDOG : public WI_LABEL_t {
public:
    MI_WATCHDOG();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_PREHEAT_ERROR : public WI_LABEL_t {
public:
    MI_PREHEAT_ERROR();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_TRIGGER_REDSCREEN : public WI_LABEL_t {
public:
    MI_TRIGGER_REDSCREEN();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

namespace detail {
using ScreenMenuErrorTest = ScreenMenu<EFooter::Off, MI_RETURN,
    MI_TRIGGER_BSOD,
    MI_STACK_OVERFLOW,
    MI_DIV0,
    MI_WATCHDOG,
    MI_PREHEAT_ERROR,
    MI_TRIGGER_REDSCREEN>;
} // namespace detail

/**
 * @brief Test Errors as BSOD, redscreens, watchdog and so on.
 * @note Enabled only in developer mode. Can be used in release build.
 */
class ScreenMenuErrorTest : public detail::ScreenMenuErrorTest {
    constexpr static const char *label = N_("TEST ERROR");

public:
    ScreenMenuErrorTest();
};
